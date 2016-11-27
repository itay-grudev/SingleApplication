// The MIT License (MIT)
//
// Copyright (c) Itay Grudev 2015 - 2016
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <cstdlib>

#include <QtCore/QProcess>
#include <QtCore/QByteArray>
#include <QtCore/QSemaphore>
#include <QtCore/QSharedMemory>
#include <QtCore/QStandardPaths>
#include <QtCore/QCryptographicHash>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#ifdef Q_OS_UNIX
    #include <signal.h>
    #include <unistd.h>
#endif

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <lmcons.h>
#endif

#include "singleapplication.h"
#include "singleapplication_p.h"

SingleApplicationPrivate::SingleApplicationPrivate( SingleApplication *q_ptr ) : q_ptr( q_ptr ) {
    server = nullptr;
    socket = nullptr;
}

SingleApplicationPrivate::~SingleApplicationPrivate()
{
    cleanUp();
}

void SingleApplicationPrivate::genBlockServerName( int timeout )
{
    QCryptographicHash appData( QCryptographicHash::Sha256 );
    appData.addData( "SingleApplication", 17 );
    appData.addData( SingleApplication::app_t::applicationName().toUtf8() );
    appData.addData( SingleApplication::app_t::organizationName().toUtf8() );
    appData.addData( SingleApplication::app_t::organizationDomain().toUtf8() );

    if( ! (options & SingleApplication::Mode::ExcludeAppVersion) ) {
        appData.addData( SingleApplication::app_t::applicationVersion().toUtf8() );
    }

    if( ! (options & SingleApplication::Mode::ExcludeAppPath) ) {
#ifdef Q_OS_WIN
        appData.addData( SingleApplication::app_t::applicationFilePath().toLower().toUtf8() );
#else
        appData.addData( SingleApplication::app_t::applicationFilePath().toUtf8() );
#endif
    }

    // User level block requires a user specific data in the hash
    if( options & SingleApplication::Mode::User ) {
#ifdef Q_OS_WIN
        Q_UNUSED(timeout);
        wchar_t username [ UNLEN + 1 ];
        // Specifies size of the buffer on input
        DWORD usernameLength = UNLEN + 1;
        if( GetUserName( username, &usernameLength ) ) {
            appData.addData( QString::fromWCharArray(username).toUtf8() );
        } else {
            appData.addData( QStandardPaths::standardLocations( QStandardPaths::HomeLocation ).join("").toUtf8() );
        }
#endif
#ifdef Q_OS_UNIX
        QString username;
        QProcess process;
        process.start( "whoami" );
        if( process.waitForFinished( timeout ) &&
            process.exitCode() == QProcess::NormalExit) {
            appData.addData( process.readLine() );
        } else {
            appData.addData( QStandardPaths::standardLocations( QStandardPaths::HomeLocation ).join("").toUtf8() );
        }
#endif
    }

    // Replace the backslash in RFC 2045 Base64 [a-zA-Z0-9+/=] to comply with
    // server naming requirements.
    blockServerName = appData.result().toBase64().replace("/", "_");
}

void SingleApplicationPrivate::startPrimary( bool resetMemory )
{
#ifdef Q_OS_UNIX
    // Handle any further termination signals to ensure the
    // QSharedMemory block is deleted even if the process crashes
    crashHandler();
#endif
    // Successful creation means that no main process exists
    // So we start a QLocalServer to listen for connections
    QLocalServer::removeServer( blockServerName );
    server = new QLocalServer();

    // Restrict access to the socket according to the
    // SingleApplication::Mode::User flag on User level or no restrictions
    if( options & SingleApplication::Mode::User ) {
      server->setSocketOptions( QLocalServer::UserAccessOption );
    } else {
      server->setSocketOptions( QLocalServer::WorldAccessOption );
    }

    server->listen( blockServerName );
    QObject::connect(
        server,
        &QLocalServer::newConnection,
        this,
        &SingleApplicationPrivate::slotConnectionEstablished
    );

    // Reset the number of connections
    memory->lock();
    InstancesInfo* inst = (InstancesInfo*)memory->data();

    if( resetMemory ){
        inst->primary = true;
        inst->secondary = 0;
    } else {
        inst->primary = true;
    }

    memory->unlock();

    instanceNumber = 0;
}

void SingleApplicationPrivate::startSecondary()
{
#ifdef Q_OS_UNIX
    // Handle any further termination signals to ensure the
    // QSharedMemory block is deleted even if the process crashes
    crashHandler();
#endif
}

void SingleApplicationPrivate::connectToPrimary( int msecs, char connectionType )
{
    // Connect to the Local Server of the Primary Instance if not already
    // connected.
    if( socket == nullptr ) {
        socket = new QLocalSocket();
    }

    // If already connected - we are done;
    if( socket->state() == QLocalSocket::ConnectedState )
        return;

    // If not connect
    if( socket->state() == QLocalSocket::UnconnectedState ||
        socket->state() == QLocalSocket::ClosingState ) {
        socket->connectToServer( blockServerName );
    }

    // Wait for being connected
    if( socket->state() == QLocalSocket::ConnectingState ) {
        socket->waitForConnected( msecs );
    }

    // Initialisation message according to the SingleApplication protocol
    if( socket->state() == QLocalSocket::ConnectedState ) {
        // Notify the parent that a new instance had been started;
        QByteArray initMsg = blockServerName.toLatin1();

        initMsg.append( connectionType );
        initMsg.append( (const char *)&instanceNumber, sizeof(quint32) );
        initMsg.append( QByteArray::number( qChecksum( initMsg.constData(), initMsg.length() ), 256) );

        socket->write( initMsg );
        socket->flush();
        socket->waitForBytesWritten( msecs );
    }
}

#ifdef Q_OS_UNIX
    void SingleApplicationPrivate::crashHandler()
    {
        // This guarantees the program will work even with multiple
        // instances of SingleApplication in different threads.
        // Which in my opinion is idiotic, but lets handle that too.
        {
            sharedMemMutex.lock();
            sharedMem.append( this );
            sharedMemMutex.unlock();
        }

        // Handle any further termination signals to ensure the
        // QSharedMemory block is deleted even if the process crashes
        signal( SIGHUP,  SingleApplicationPrivate::terminate ); // 1
        signal( SIGINT,  SingleApplicationPrivate::terminate ); // 2
        signal( SIGQUIT, SingleApplicationPrivate::terminate ); // 3
        signal( SIGILL,  SingleApplicationPrivate::terminate ); // 4
        signal( SIGABRT, SingleApplicationPrivate::terminate ); // 6
        signal( SIGFPE,  SingleApplicationPrivate::terminate ); // 8
        signal( SIGBUS,  SingleApplicationPrivate::terminate ); // 10
        signal( SIGSEGV, SingleApplicationPrivate::terminate ); // 11
        signal( SIGSYS,  SingleApplicationPrivate::terminate ); // 12
        signal( SIGPIPE, SingleApplicationPrivate::terminate ); // 13
        signal( SIGALRM, SingleApplicationPrivate::terminate ); // 14
        signal( SIGTERM, SingleApplicationPrivate::terminate ); // 15
        signal( SIGXCPU, SingleApplicationPrivate::terminate ); // 24
        signal( SIGXFSZ, SingleApplicationPrivate::terminate ); // 25
    }

    void SingleApplicationPrivate::terminate( int signum )
    {
        while( ! sharedMem.empty() ) {
            delete sharedMem.back();
            sharedMem.pop_back();
        }
        ::exit( 128 + signum );
    }

    QList<SingleApplicationPrivate*> SingleApplicationPrivate::sharedMem;
    QMutex SingleApplicationPrivate::sharedMemMutex;
#endif

void SingleApplicationPrivate::cleanUp() {
    if( socket != nullptr ) {
        socket->close();
        delete socket;
    }
    memory->lock();
    InstancesInfo* inst = (InstancesInfo*)memory->data();
    if( server != nullptr ) {
        server->close();
        inst->primary = false;
    }
    memory->unlock();
    delete memory;
}

/**
 * @brief Executed when a connection has been made to the LocalServer
 */
void SingleApplicationPrivate::slotConnectionEstablished()
{
    Q_Q(SingleApplication);

    QLocalSocket *socket = server->nextPendingConnection();

    // Verify that the new connection follows the SingleApplication protocol
    char connectionType;
    quint32 instanceId;
    QByteArray initMsg, tmp;
    bool invalidConnection = false;
    if( socket->waitForReadyRead( 100 ) ) {
        tmp = socket->read( blockServerName.length() );
        // Verify that the socket data start with blockServerName
        if( tmp == blockServerName.toLatin1() ) {
            initMsg = tmp;
            tmp = socket->read(1);
            // Verify that the next charecter is N/S/R (connecion type)
            // Stands for New Instance/Secondary Instance/Reconnect
            if( tmp == "N" || tmp == "S" || tmp == "R" ) {
                connectionType = tmp.at(0);
                initMsg += tmp;
                tmp = socket->read( sizeof(quint32) );
                const char * data = tmp.constData();
                instanceId = (quint32)*data;
                initMsg += tmp;
                // Verify the checksum of the initMsg
                QByteArray checksum = QByteArray::number(
                    qChecksum( initMsg.constData(), initMsg.length() ),
                    256
                );
                tmp = socket->read( checksum.length() );
                if( checksum != tmp ) {
                    invalidConnection = true;
                }
            } else {
                invalidConnection = true;
            }
        } else {
            invalidConnection = true;
        }
    } else {
        invalidConnection = true;
    }

    if( invalidConnection ) {
        socket->close();
        delete socket;
        return;
    }

    QObject::connect(
        socket,
        &QLocalSocket::aboutToClose,
        this,
        [socket, instanceId, this]() {
            Q_EMIT this->slotClientConnectionClosed( socket, instanceId );
        }
    );

    QObject::connect(
        socket,
        &QLocalSocket::readyRead,
        this,
        [socket, instanceId, this]() {
            Q_EMIT this->slotDataAvailable( socket, instanceId );
        }
    );

    if( connectionType == 'N' || (
            connectionType == 'S' &&
            options & SingleApplication::Mode::SecondaryNotification
        )
    ) {
        Q_EMIT q->instanceStarted();
    }

    if( socket->bytesAvailable() > 0 ) {
        Q_EMIT this->slotDataAvailable( socket, instanceId );
    }
}

void SingleApplicationPrivate::slotDataAvailable( QLocalSocket *socket, quint32 instanceId )
{
    Q_Q(SingleApplication);
    Q_EMIT q->receivedMessage( instanceId, socket->readAll() );
}

void SingleApplicationPrivate::slotClientConnectionClosed( QLocalSocket *socket, quint32 instanceId )
{
    if( socket->bytesAvailable() > 0 )
        Q_EMIT slotDataAvailable( socket, instanceId  );
    socket->deleteLater();
}

/**
 * @brief Constructor. Checks and fires up LocalServer or closes the program
 * if another instance already exists
 * @param argc
 * @param argv
 * @param {bool} allowSecondaryInstances
 */
SingleApplication::SingleApplication( int &argc, char *argv[], bool allowSecondary, Options options, int timeout )
    : app_t( argc, argv ), d_ptr( new SingleApplicationPrivate( this ) )
{
    Q_D(SingleApplication);

    // Store the current mode of the program
    d->options = options;

    // Generating an application ID used for identifying the shared memory
    // block and QLocalServer
    d->genBlockServerName( timeout );

    // Guarantee thread safe behaviour with a shared memory block. Also by
    // explicitly attaching it and then deleting it we make sure that the
    // memory is deleted even if the process had crashed on Unix.
#ifdef Q_OS_UNIX
    d->memory = new QSharedMemory( d->blockServerName );
    d->memory->attach();
    delete d->memory;
#endif
    d->memory = new QSharedMemory( d->blockServerName );

    // Create a shared memory block
    if( d->memory->create( sizeof( InstancesInfo ) ) ) {
        d->startPrimary( true );
        return;
    } else {
        // Attempt to attach to the memory segment
        if( d->memory->attach() ) {
            d->memory->lock();
            InstancesInfo* inst = (InstancesInfo*)d->memory->data();

            if( ! inst->primary ) {
                d->startPrimary( false );
                d->memory->unlock();
                return;
            }

            // Check if another instance can be started
            if( allowSecondary ) {
                inst->secondary += 1;
                d->instanceNumber = inst->secondary;
                d->startSecondary();
                if( d->options & Mode::SecondaryNotification ) {
                    d->connectToPrimary( timeout, 'S' );
                }
                d->memory->unlock();
                return;
            }

            d->memory->unlock();
        }
    }

    d->connectToPrimary( timeout, 'N' );
    delete d;
    ::exit( EXIT_SUCCESS );
}

/**
 * @brief Destructor
 */
SingleApplication::~SingleApplication()
{
    Q_D(SingleApplication);
    delete d;
}

bool SingleApplication::isPrimary()
{
    Q_D(SingleApplication);
    return d->server != nullptr;
}

bool SingleApplication::isSecondary()
{
    Q_D(SingleApplication);
    return d->server == nullptr;
}

quint32 SingleApplication::instanceId()
{
    Q_D(SingleApplication);
    return d->instanceNumber;
}

bool SingleApplication::sendMessage( QByteArray message, int timeout )
{
    Q_D(SingleApplication);

    // Nobody to connect to
    if( isPrimary() ) return false;

    // Make sure the socket is connected
    d->connectToPrimary( timeout, 'R' );

    d->socket->write( message );
    bool dataWritten = d->socket->flush();
    d->socket->waitForBytesWritten( timeout );
    return dataWritten;
}

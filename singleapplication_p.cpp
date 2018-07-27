// The MIT License (MIT)
//
// Copyright (c) Itay Grudev 2015 - 2018
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

//
//  W A R N I N G !!!
//  -----------------
//
// This file is not part of the SingleApplication API. It is used purely as an
// implementation detail. This header file may change from version to
// version without notice, or may even be removed.
//

#include <cstdlib>
#include <cstddef>

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QByteArray>
#include <QtCore/QSemaphore>
#include <QtCore/QDataStream>
#include <QtCore/QStandardPaths>
#include <QtCore/QCryptographicHash>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include "singleapplication.h"
#include "singleapplication_p.h"

#ifdef Q_OS_UNIX
    #include <signal.h>
    #include <unistd.h>
#endif

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <lmcons.h>
#endif

SingleApplicationPrivate::SingleApplicationPrivate( SingleApplication *q_ptr )
    : q_ptr( q_ptr )
{
    server = nullptr;
    socket = nullptr;
}

SingleApplicationPrivate::~SingleApplicationPrivate()
{
    if( socket != nullptr ) {
        socket->close();
        delete socket;
    }

    memory->lock();
    InstancesInfo* inst = static_cast<InstancesInfo*>(memory->data());
    if( server != nullptr ) {
        server->close();
        delete server;
        inst->primary = false;
        inst->primaryPid = -1;
        inst->checksum = blockChecksum();
    }
    memory->unlock();

    delete memory;
}

void SingleApplicationPrivate::genBlockServerName()
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
        if( GetUserNameW( username, &usernameLength ) ) {
            appData.addData( QString::fromWCharArray(username).toUtf8() );
        } else {
            appData.addData( QStandardPaths::standardLocations( QStandardPaths::HomeLocation ).join("").toUtf8() );
        }
#endif
#ifdef Q_OS_UNIX
        QProcess process;
        process.start( "whoami" );
        if( process.waitForFinished( 100 ) &&
            process.exitCode() == QProcess::NormalExit) {
            appData.addData( process.readLine() );
        } else {
            appData.addData(
                QDir(
                    QStandardPaths::standardLocations( QStandardPaths::HomeLocation ).first()
                ).absolutePath().toUtf8()
            );
        }
#endif
    }

    // Replace the backslash in RFC 2045 Base64 [a-zA-Z0-9+/=] to comply with
    // server naming requirements.
    blockServerName = appData.result().toBase64().replace("/", "_");
}

void SingleApplicationPrivate::initializeMemoryBlock()
{
    InstancesInfo* inst = static_cast<InstancesInfo*>( memory->data() );
    inst->primary = false;
    inst->secondary = 0;
    inst->primaryPid = -1;
    inst->checksum = blockChecksum();
}

void SingleApplicationPrivate::startPrimary()
{
    Q_Q(SingleApplication);

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
    InstancesInfo* inst = static_cast <InstancesInfo*>( memory->data() );

    inst->primary = true;
    inst->primaryPid = q->applicationPid();
    inst->checksum = blockChecksum();

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

void SingleApplicationPrivate::connectToPrimary( int msecs, ConnectionType connectionType )
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
        QByteArray initMsg;
        QDataStream writeStream(&initMsg, QIODevice::WriteOnly);
        writeStream.setVersion(QDataStream::Qt_5_6);
        writeStream << blockServerName.toLatin1();
        writeStream << static_cast<quint8>(connectionType);
        writeStream << instanceNumber;
        quint16 checksum = qChecksum(initMsg.constData(), static_cast<quint32>(initMsg.length()));
        writeStream << checksum;

        // The header indicates the message length that follows
        QByteArray header;
        QDataStream headerStream(&header, QIODevice::WriteOnly);
        headerStream.setVersion(QDataStream::Qt_5_6);
        headerStream << static_cast <quint64>( initMsg.length() );

        socket->write( header );
        socket->write( initMsg );
        socket->flush();
        socket->waitForBytesWritten( msecs );
    }
}

quint16 SingleApplicationPrivate::blockChecksum()
{
    return qChecksum(
       static_cast <const char *>( memory->data() ),
       offsetof( InstancesInfo, checksum )
   );
}

qint64 SingleApplicationPrivate::primaryPid()
{
    qint64 pid;

    memory->lock();
    InstancesInfo* inst = static_cast<InstancesInfo*>( memory->data() );
    pid = inst->primaryPid;
    memory->unlock();

    return pid;
}

#ifdef Q_OS_UNIX
    void SingleApplicationPrivate::crashHandler()
    {
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

    [[noreturn]] void SingleApplicationPrivate::terminate( int signum )
    {
        delete (static_cast <SingleApplication*>( QCoreApplication::instance() ))->d_ptr;
        ::exit( 128 + signum );
    }
#endif

/**
 * @brief Executed when a connection has been made to the LocalServer
 */
void SingleApplicationPrivate::slotConnectionEstablished()
{
    Q_Q(SingleApplication);

    QLocalSocket *nextConnSocket = server->nextPendingConnection();

    quint32 instanceId = 0;
    ConnectionType connectionType = InvalidConnection;
    if( nextConnSocket->waitForReadyRead( 100 ) ) {
        // read the fields in same order and format as written
        QDataStream headerStream(nextConnSocket);
        headerStream.setVersion( QDataStream::Qt_5_6 );

        // Read the header to know the message length
        quint64 msgLen = 0;
        headerStream >> msgLen;

        if( msgLen >= sizeof( quint16 ) ) {
           // Read the message body
           QByteArray msgBytes = nextConnSocket->read(msgLen);
           QDataStream readStream(msgBytes);
           readStream.setVersion( QDataStream::Qt_5_6 );

           // server name
           QByteArray latin1Name;
           readStream >> latin1Name;

           // connection type
           quint8 connType = InvalidConnection;
           readStream >> connType;
           connectionType = static_cast <ConnectionType>( connType );

           // instance id
           readStream >> instanceId;

           // checksum
           quint16 msgChecksum = 0;
           readStream >> msgChecksum;

           const quint16 actualChecksum = qChecksum(msgBytes.constData(), static_cast<quint32>(msgBytes.length() - sizeof(quint16)));

           if (readStream.status() != QDataStream::Ok || QLatin1String(latin1Name) != blockServerName || msgChecksum != actualChecksum) {
             connectionType = InvalidConnection;
           }
        }
    }

    if( connectionType == InvalidConnection ) {
        nextConnSocket->close();
        delete nextConnSocket;
        return;
    }

    QObject::connect(
        nextConnSocket,
        &QLocalSocket::aboutToClose,
        this,
        [nextConnSocket, instanceId, this]() {
            Q_EMIT this->slotClientConnectionClosed( nextConnSocket, instanceId );
        }
    );

    QObject::connect(
        nextConnSocket,
        &QLocalSocket::readyRead,
        this,
        [nextConnSocket, instanceId, this]() {
            Q_EMIT this->slotDataAvailable( nextConnSocket, instanceId );
        }
    );

    if( connectionType == NewInstance || (
            connectionType == SecondaryInstance &&
            options & SingleApplication::Mode::SecondaryNotification
        )
    ) {
        Q_EMIT q->instanceStarted();
    }

    if( nextConnSocket->bytesAvailable() > 0 ) {
        Q_EMIT this->slotDataAvailable( nextConnSocket, instanceId );
    }
}

void SingleApplicationPrivate::slotDataAvailable( QLocalSocket *dataSocket, quint32 instanceId )
{
    Q_Q(SingleApplication);
    Q_EMIT q->receivedMessage( instanceId, dataSocket->readAll() );
}

void SingleApplicationPrivate::slotClientConnectionClosed( QLocalSocket *closedSocket, quint32 instanceId )
{
    if( closedSocket->bytesAvailable() > 0 )
        Q_EMIT slotDataAvailable( closedSocket, instanceId  );
    closedSocket->deleteLater();
}

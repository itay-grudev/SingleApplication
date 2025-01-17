// Copyright (c) Itay Grudev 2023
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// Permission is not granted to use this software or any of the associated files
// as sample data for the purposes of building machine learning models.
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

#include <QtCore/QElapsedTimer>
#include <QtCore/QByteArray>
#include <QtCore/QSharedMemory>
#include <QtCore/QDebug>

#include <error.h>

#include "singleapplication.h"
#include "singleapplication_p.h"

/**
 * @brief Constructor. Checks and fires up LocalServer or closes the program
 * if another instance already exists
 * @param argc
 * @param argv
 * @param allowSecondary Whether to enable secondary instance support
 * @param options Optional flags to toggle specific behaviour
 * @param timeout Maximum time blocking functions are allowed during app load
 */
SingleApplication::SingleApplication( int &argc, char *argv[], bool allowSecondary, Options options, int timeout, const QString &userData )
    : app_t( argc, argv ), d_ptr( new SingleApplicationPrivate( this ) )
{
    Q_D( SingleApplication );

    // Keep track of the initialization time of SingleApplication
    QElapsedTimer time;
    time.start();

    // Store the current mode of the program
    d->options = options;

    // Add any unique user data
    if ( ! userData.isEmpty() )
        d->addAppData( userData );

    // Generating an application ID used for identifying the shared memory
    // block and QLocalServer
    d->genBlockServerName();

//<<<<<<< v4.0
    while( time.elapsed() < timeout ){
        if( d->connectToPrimary( (timeout - time.elapsed()) * 2 / 3 )){
            if( ! allowSecondary ) // If we are operating in single instance mode - terminate the program
                ::exit( EXIT_SUCCESS );
/*=======
    // To mitigate QSharedMemory issues with large amount of processes
    // attempting to attach at the same time
    SingleApplicationPrivate::randomSleep();

#ifdef Q_OS_UNIX
    // By explicitly attaching it and then deleting it we make sure that the
    // memory is deleted even after the process has crashed on Unix.
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    d->memory = new QSharedMemory( QNativeIpcKey( d->blockServerName ) );
#else
    d->memory = new QSharedMemory( d->blockServerName );
#endif
    d->memory->attach();
    delete d->memory;
#endif
    // Guarantee thread safe behaviour with a shared memory block.
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    d->memory = new QSharedMemory( QNativeIpcKey( d->blockServerName ) );
#else
    d->memory = new QSharedMemory( d->blockServerName );
#endif
>>>>>> master */

            d->notifySecondaryStart( timeout );
            return;
        } else {
            // Report unexpected errors
            switch( d->socket->error() ){
            case QLocalSocket::SocketAccessError:
            case QLocalSocket::SocketResourceError:
            case QLocalSocket::DatagramTooLargeError:
            case QLocalSocket::UnsupportedSocketOperationError:
            case QLocalSocket::OperationError:
            case QLocalSocket::UnknownSocketError:
                qCritical() << "SingleApplication:" << d->socket->errorString();
                qDebug() << "SingleApplication:" << "Falling back to primary instance";
                break;
            default:
                break;
            }
            // If No server is listening then this is a promoted to a primary instance.
            if( d->startPrimary( timeout ))
                return;
        }
    }

    qFatal( "SingleApplication: Did not manage to initialize within the allocated time1out." );
}

SingleApplication::~SingleApplication()
{
    Q_D( SingleApplication );
    delete d;
}

/**
 * Checks if the current application instance is primary.
 * @return Returns true if the instance is primary, false otherwise.
 */
bool SingleApplication::isPrimary() const
{
    Q_D( const SingleApplication );
    return d->server != nullptr;
}

/**
 * Checks if the current application instance is secondary.
 * @return Returns true if the instance is secondary, false otherwise.
 */
bool SingleApplication::isSecondary() const
{
    Q_D( const SingleApplication );
    return d->server == nullptr;
}

/**
 * Allows you to identify an instance by returning unique consecutive instance
 * ids. It is reset when the first (primary) instance of your app starts and
 * only incremented afterwards.
 * @return Returns a unique instance id.
 */
quint32 SingleApplication::instanceId() const
{
    Q_D( const SingleApplication );
    return d->instanceNumber;
}

/**
 * Returns the OS PID (Process Identifier) of the process running the primary
 * instance. Especially useful when SingleApplication is coupled with OS.
 * specific APIs.
 * @return Returns the primary instance PID.
 */
qint64 SingleApplication::primaryPid() const
{
    Q_D( const SingleApplication );
    return d->primaryPid();
}

/**
 * Returns the username the primary instance is running as.
 * @return Returns the username the primary instance is running as.
 */
QString SingleApplication::primaryUser() const
{
    Q_D( const SingleApplication );
    return d->primaryUser();
}

/**
 * Returns the username the current instance is running as.
 * @return Returns the username the current instance is running as.
 */
QString SingleApplication::currentUser() const
{
    return SingleApplicationPrivate::getUsername();
}

/**
 * Sends message to the Primary Instance.
 * @param message The message to send.
 * @param timeout the maximum timeout in milliseconds for blocking functions.
 * @param sendMode mode of operation
 * @return true if the message was received successfuly, false otherwise.
 */
bool SingleApplication::sendMessage( const QByteArray &messageBody, int timeout )
{
    Q_D( SingleApplication );

    // Nobody to connect to
    if( isPrimary() ) return false;

//    SingleApplicationMessage message( SingleApplicationMessage::NewInstance, 0, messageBody );
//    return d->sendApplicationMessage( message , timeout );


    return d->sendApplicationMessage( SingleApplication::MessageType::InstanceMessage, messageBody, timeout );
}

QStringList SingleApplication::userData() const
{
    Q_D( const SingleApplication );
    return d->appData();
}

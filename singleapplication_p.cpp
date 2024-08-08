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
#include <QtCore/QThread>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QElapsedTimer>
#include <QtCore/QCryptographicHash>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include "message_coder.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QtCore/QRandomGenerator>
#else
#include <QtCore/QDateTime>
#endif

#include "singleapplication.h"
#include "singleapplication_p.h"

#ifdef Q_OS_UNIX
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif

#ifdef Q_OS_WIN
    #ifndef NOMINMAX
        #define NOMINMAX 1
    #endif
    #include <windows.h>
    #include <lmcons.h>
#endif

SingleApplicationPrivate::SingleApplicationPrivate( SingleApplication *q_ptr )
    : q_ptr( q_ptr ), server( nullptr ), socket( nullptr ), instanceNumber( 0 ), connectionMap()
{
}

SingleApplicationPrivate::~SingleApplicationPrivate()
{
    if( socket != nullptr ){
        socket->close();
        delete socket;
    }
}

QString SingleApplicationPrivate::getUsername()
{
#ifdef Q_OS_WIN
      wchar_t username[UNLEN + 1];
      // Specifies size of the buffer on input
      DWORD usernameLength = UNLEN + 1;
      if( GetUserNameW( username, &usernameLength ) )
          return QString::fromWCharArray( username );
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
      return QString::fromLocal8Bit( qgetenv( "USERNAME" ) );
#else
      return qEnvironmentVariable( "USERNAME" );
#endif
#endif
#ifdef Q_OS_UNIX
      QString username;
      uid_t uid = geteuid();
      struct passwd *pw = getpwuid( uid );
      if( pw )
          username = QString::fromLocal8Bit( pw->pw_name );
      if ( username.isEmpty() ){
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
          username = QString::fromLocal8Bit( qgetenv( "USER" ) );
#else
          username = qEnvironmentVariable( "USER" );
#endif
      }
      return username;
#endif
}

void SingleApplicationPrivate::genBlockServerName()
{
#ifdef Q_OS_MACOS
    // Maximum key size on macOS is PSHMNAMLEN (31).
    QCryptographicHash appData( QCryptographicHash::Md5 );
#else
    QCryptographicHash appData( QCryptographicHash::Sha256 );
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
    appData.addData( "SingleApplication", 17 );
#else
    appData.addData( QByteArrayView{"SingleApplication"} );
#endif
    appData.addData( SingleApplication::app_t::applicationName().toUtf8() );
    appData.addData( SingleApplication::app_t::organizationName().toUtf8() );
    appData.addData( SingleApplication::app_t::organizationDomain().toUtf8() );

    if ( ! appDataList.isEmpty() )
        appData.addData( appDataList.join(QString()).toUtf8() );

    if( ! (options & SingleApplication::Mode::ExcludeAppVersion) ){
        appData.addData( SingleApplication::app_t::applicationVersion().toUtf8() );
    }

    if( ! (options & SingleApplication::Mode::ExcludeAppPath) ){
#if defined(Q_OS_WIN)
        appData.addData( SingleApplication::app_t::applicationFilePath().toLower().toUtf8() );
#elif defined(Q_OS_LINUX)
        // If the application is running as an AppImage then the APPIMAGE env var should be used
        // instead of applicationPath() as each instance is launched with its own executable path
        const QByteArray appImagePath = qgetenv( "APPIMAGE" );
        if( appImagePath.isEmpty() ){ // Not running as AppImage: use path to executable file
            appData.addData( SingleApplication::app_t::applicationFilePath().toUtf8() );
        } else { // Running as AppImage: Use absolute path to AppImage file
            appData.addData( appImagePath );
        };
#else
        appData.addData( SingleApplication::app_t::applicationFilePath().toUtf8() );
#endif
    }

    // User level block requires a user specific data in the hash
    if( options & SingleApplication::Mode::User ){
        appData.addData( getUsername().toUtf8() );
    }

    // Replace the backslash in RFC 2045 Base64 [a-zA-Z0-9+/=] to comply with
    // server naming requirements.
    blockServerName = QString::fromUtf8(appData.result().toBase64().replace("/", "_"));
}

bool SingleApplicationPrivate::startPrimary( uint timeout )
{
    // Successful creation means that no main process exists
    // So we start a QLocalServer to listen for connections
    QLocalServer::removeServer( blockServerName );
    server = new QLocalServer();

    // Restrict access to the socket according to the
    // SingleApplication::Mode::User flag on User level or no restrictions
    if( options & SingleApplication::Mode::User ){
        server->setSocketOptions( QLocalServer::UserAccessOption );
    } else {
        server->setSocketOptions( QLocalServer::WorldAccessOption );
    }

    QObject::connect(
        server,
        &QLocalServer::newConnection,
        this,
        &SingleApplicationPrivate::slotConnectionEstablished
    );

    if( server->listen( blockServerName ) )
        return true;

    delete server;
    return false;
}

bool SingleApplicationPrivate::connectToPrimary( uint timeout ){
    if( socket == nullptr )
        socket = new QLocalSocket( this );

    if( socket->state() == QLocalSocket::ConnectedState )
        return true;

    if( socket->state() != QLocalSocket::ConnectingState )
        socket->connectToServer( blockServerName );

    return socket->waitForConnected( timeout );
}

void SingleApplicationPrivate::notifySecondaryStart( uint timeout )
{
//    SingleApplicationMessage message( SingleApplicationMessage::NewInstance, 0, QByteArray() );
//    sendApplicationMessage( message, timeout );
    sendApplicationMessage( SingleApplication::MessageType::NewInstance, QByteArray(), timeout );
}

//bool SingleApplicationPrivate::sendApplicationMessage( SingleApplicationMessage message, uint timeout )
//{
//    SingleApplicationMessage response;
//    return sendApplicationMessage( message, timeout, response );
//}

bool SingleApplicationPrivate::sendApplicationMessage( SingleApplication::MessageType messageType, QByteArray content, uint timeout )
{
    QElapsedTimer elapsedTime;
    elapsedTime.start();

    if( ! connectToPrimary( timeout * 2 / 3 ))
        return false;

    MessageCoder coder( socket );
    coder.sendMessage( messageType, instanceNumber, content );

    socket->flush();
    return socket->waitForBytesWritten( qMax(timeout - elapsedTime.elapsed(), 1) );

    // TODO: Wait for an ACK message
//    if( socket->waitForReadyRead( timeout )){
//        QByteArray responseBytes = socket->readAll();
//        response = SingleApplicationMessage( socket->readAll() );
//
//        // The response message is invalid
//        if( response.invalid )
//            return false;
//
//        // The response message didn't contain the primary instance id
//        if( response.instanceId != 0 )
//            return false;
//
//        // This isn't an acknowledge message
//        if( response.type != SingleApplicationMessage::Acknowledge )
//            return false;
//
//        return true;
//    }
//
//    return false;
}

qint64 SingleApplicationPrivate::primaryPid() const
{
    qint64 pid;

    // TODO: Reimplement with message response

    return pid;
}

QString SingleApplicationPrivate::primaryUser() const
{
    QByteArray username;

    // TODO: Reimplement with message response

    return QString::fromUtf8( username );
}

/**
 * @brief Executed when a connection has been made to the LocalServer
 */
void SingleApplicationPrivate::slotConnectionEstablished()
{
    QLocalSocket *nextConnSocket = server->nextPendingConnection();

    connectionMap.insert( nextConnSocket, ConnectionInfo() );
    connectionMap[nextConnSocket].coder = new MessageCoder( nextConnSocket );

    QObject::connect( nextConnSocket, &QLocalSocket::disconnected, nextConnSocket, &QLocalSocket::deleteLater );

    QObject::connect( nextConnSocket, &QLocalSocket::destroyed, this,
        [nextConnSocket, this](){
            connectionMap.remove( nextConnSocket );
        }
    );
}

void SingleApplicationPrivate::randomSleep()
{
#if QT_VERSION >= QT_VERSION_CHECK( 5, 10, 0 )
    QThread::msleep( QRandomGenerator::global()->bounded( 8u, 18u ));
#else
    qsrand( QDateTime::currentMSecsSinceEpoch() % std::numeric_limits<uint>::max() );
    QThread::msleep( qrand() % 11 + 8);
#endif
}

void SingleApplicationPrivate::addAppData(const QString &data)
{
    appDataList.push_back(data);
}

QStringList SingleApplicationPrivate::appData() const
{
    return appDataList;
}

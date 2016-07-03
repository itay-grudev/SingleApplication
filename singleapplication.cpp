#include <cstdlib>

#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QtCore/QSharedMemory>
#include <QtCore/QBuffer>
#include <QtCore/QEventLoop>
#include <QtCore/QJsonObject>
#include <QtCore/QDataStream>
#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

#ifdef Q_OS_UNIX
    #include <signal.h>
    #include <unistd.h>
#endif

#include "singleapplication.h"


static const QString COMMAND = "command";
static const QString COM_SHOW = "show";


bool SingleApplication::_allowSecondary = false;

struct InstancesInfo {
    bool primary;
    uint8_t secondary;
};

class SingleApplicationPrivate {
public:
    Q_DECLARE_PUBLIC(SingleApplication)

    SingleApplicationPrivate( SingleApplication *q_ptr ) : q_ptr( q_ptr ) {
        server = NULL;
    }

    ~SingleApplicationPrivate()
    {
       cleanUp();
    }

    void startPrimary( bool resetMemory )
    {
        Q_Q(SingleApplication);
#ifdef Q_OS_UNIX
        // Handle any further termination signals to ensure the
        // QSharedMemory block is deleted even if the process crashes
        crashHandler();
#endif
        // Successful creation means that no main process exists
        // So we start a QLocalServer to listen for connections
        QLocalServer::removeServer( memory->key() );
        server = new QLocalServer();
        server->listen( memory->key() );
        QObject::connect(
            server,
            &QLocalServer::newConnection,
            q,
            &SingleApplication::slotConnectionEstablished
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
    }

    void startSecondary()
    {
#ifdef Q_OS_UNIX
        // Handle any further termination signals to ensure the
        // QSharedMemory block is deleted even if the process crashes
        crashHandler();
#endif

        notifyPrimary();
    }

    void notifyPrimary()
    {
        Q_Q(SingleApplication);
        // Connect to the Local Server of the main process to notify it
        // that a new process had been started
        QJsonObject obj{ {COMMAND, COM_SHOW} };
        q->sendCommand(obj);
    }

#ifdef Q_OS_UNIX
    void crashHandler()
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
        signal( SIGHUP, SingleApplicationPrivate::terminate );   // 1
        signal( SIGINT,  SingleApplicationPrivate::terminate );  // 2
        signal( SIGQUIT,  SingleApplicationPrivate::terminate ); // 3
        signal( SIGILL,  SingleApplicationPrivate::terminate );  // 4
        signal( SIGABRT, SingleApplicationPrivate::terminate );  // 6
        signal( SIGFPE,  SingleApplicationPrivate::terminate );  // 8
        signal( SIGBUS,  SingleApplicationPrivate::terminate );  // 10
        signal( SIGSEGV, SingleApplicationPrivate::terminate );  // 11
        signal( SIGSYS, SingleApplicationPrivate::terminate );   // 12
        signal( SIGPIPE, SingleApplicationPrivate::terminate );  // 13
        signal( SIGALRM, SingleApplicationPrivate::terminate );  // 14
        signal( SIGTERM, SingleApplicationPrivate::terminate );  // 15
        signal( SIGXCPU, SingleApplicationPrivate::terminate );  // 24
        signal( SIGXFSZ, SingleApplicationPrivate::terminate );  // 25
    }

    static void terminate( int signum )
    {
        while( ! sharedMem.empty() ) {
            delete sharedMem.back();
            sharedMem.pop_back();
        }
        ::exit( 128 + signum );
    }

    static QList<SingleApplicationPrivate*> sharedMem;
    static QMutex sharedMemMutex;
#endif

    void cleanUp() {
        memory->lock();
        InstancesInfo* inst = (InstancesInfo*)memory->data();
        if( server != NULL ) {
            server->close();
            inst->primary = false;
        } else {
            if( inst->secondary > 0 )
                inst->secondary -= 1;
        }
        memory->unlock();
        delete memory;
    }

    QSharedMemory *memory;
    SingleApplication *q_ptr;
    QLocalServer *server;
};

#ifdef Q_OS_UNIX
    QList<SingleApplicationPrivate*> SingleApplicationPrivate::sharedMem;
    QMutex SingleApplicationPrivate::sharedMemMutex;
#endif

/**
 * @brief Constructor. Checks and fires up LocalServer or closes the program
 * if another instance already exists
 * @param argc
 * @param argv
 */
SingleApplication::SingleApplication( int &argc, char *argv[], uint8_t secondaryInstances )
    : app_t( argc, argv ), d_ptr( new SingleApplicationPrivate( this ) )
{
    Q_D(SingleApplication);

    // Check command line arguments for the force primary and secondary flags
#ifdef Q_OS_UNIX
    bool forcePrimary = false;
#endif
    bool secondary = allowSecondary();
    for( int i = 0; i < argc; ++i ) {
        if( strcmp( argv[i], "--secondary" ) == 0 ) {
            secondary = true;
#ifndef Q_OS_UNIX
            break;
#endif
        }
#ifdef Q_OS_UNIX
        if( strcmp( argv[i], "--primary" ) == 0 ) {
            secondary = false;
            forcePrimary = true;
            break;
        }
#endif
    }

    QString serverName = app_t::organizationName() + app_t::applicationName();
    serverName.replace( QRegExp("[^\\w\\-. ]"), "" );

    // Guarantee thread safe behaviour with a shared memory block. Also by
    // attaching to it and deleting it we make sure that the memory is deleted
    // even if the process had crashed
    d->memory = new QSharedMemory( serverName );
    d->memory->attach();
    delete d->memory;
    d->memory = new QSharedMemory( serverName );

    // Create a shared memory block with a minimum size of 1 byte
#ifdef Q_OS_UNIX
    if( d->memory->create( sizeof(InstancesInfo) ) || forcePrimary ) {
#else
    if( d->memory->create( sizeof(InstancesInfo) ) ) {
#endif
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
            if( secondary && inst->secondary < secondaryInstances ) {
                inst->secondary += 1;
                d->startSecondary();
                d->memory->unlock();
                return;
            }

            d->memory->unlock();
        }
    }

    d->notifyPrimary();
    delete d;
    ::exit(EXIT_SUCCESS);
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
    return d->server != NULL;
}

bool SingleApplication::isSecondary()
{
    Q_D(SingleApplication);
    return d->server == NULL;
}

bool SingleApplication::allowSecondary()
{
    return _allowSecondary;
}

void SingleApplication::setAllowSecondary(bool allow)
{
    _allowSecondary = allow;
}

void SingleApplication::sendCommand(const QJsonObject &obj)
{
    Q_D(SingleApplication);

    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    QDataStream out(&buffer);
    out << obj.toVariantMap();

    QLocalSocket* soc = new QLocalSocket;
    connect(soc, &QLocalSocket::disconnected, soc, &QLocalSocket::deleteLater);
    connect(soc, SIGNAL(error(QLocalSocket::LocalSocketError)), soc, SLOT(deleteLater()));

    connect(soc, &QLocalSocket::connected, [soc, &buffer]() {
        soc->write(buffer.data().constData(), buffer.size());
        soc->flush();
        soc->disconnectFromServer();
    });

    QEventLoop e;
    connect(soc, &QLocalSocket::destroyed, &e, &QEventLoop::quit);

    soc->connectToServer(d->memory->key(), QLocalSocket::WriteOnly);
    e.exec();
}

/**
 * @brief Executed when a connection has been made to the LocalServer
 */
void SingleApplication::slotConnectionEstablished()
{
    Q_D(SingleApplication);

    QLocalSocket *socket = d->server->nextPendingConnection();
    connect(socket, &QLocalSocket::readChannelFinished, this, &SingleApplication::dataReady);
    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

void SingleApplication::dataReady()
{
    QLocalSocket* socket = static_cast<QLocalSocket*>(sender());

    QBuffer buffer;
    QDataStream in(&buffer);

    QByteArray ba = socket->readAll();
    socket->deleteLater();

    if(ba.size() == 0)
        return;

    buffer.setData(ba);
    buffer.open(QBuffer::ReadOnly);

    QVariantMap m;
    in >> m;

    QJsonObject obj = QJsonObject::fromVariantMap(m);

    if(!obj.isEmpty() && obj.contains(COMMAND)) {
        const QString com = obj.value(COMMAND).toString();
        if(com == COM_SHOW) {
            Q_EMIT showUp();
            return;
        }

        Q_EMIT command(obj);
    }
}

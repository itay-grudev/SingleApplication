#include "singleapplication.h"

/**
 * @brief SingleApplication::SingleApplication
 *  Constructor. Checks and fires up LocalServer or closes the program
 *  if another instance already exists
 * @param argc
 * @param argv
 */
SingleApplication::SingleApplication(int argc, char *argv[]) :
  QGuiApplication(argc, argv)
{
  _shouldContinue = false; // By default this is not the main process

  socket = new QLocalSocket();

  // Attempt to connect to the LocalServer
  socket->connectToServer(LOCAL_SERVER_NAME);
  if(socket->waitForConnected(100)){
    socket->write("CMD:showUp");
    socket->flush();
    QThread::msleep(100);
    socket->close();
  } else {
    // The attempt was insuccessful, so we continue the program
    _shouldContinue = true;
    server = new LocalServer();
    server->start();
    QObject::connect(server, SIGNAL(showUp()), this, SLOT(slotShowUp()));
  }
}

/**
 * @brief SingleApplication::~SingleApplication
 *  Destructor
 */
Application::~SingleApplication()
{
  if(_shouldContinue){
    server->terminate();
  }
}

/**
 * @brief SingleApplication::shouldContinue
 *  Weather the program should be terminated
 * @return bool
 */
bool SingleApplication::shouldContinue()
{
  return _shouldContinue;
}

/**
 * @brief SingleApplication::slotShowUp
 *  Executed when the showUp command is sent to LocalServer
 */
void SingleApplication::slotShowUp()
{
  emit showUp();
}

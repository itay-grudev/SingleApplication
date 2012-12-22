#include "localserver.h"

#include <QFile>
#include <QStringList>

/**
 * @brief LocalServer::LocalServer
 *  Constructor
 */
LocalServer::LocalServer()
{

}

/**
 * @brief LocalServer::~LocalServer
 *  Destructor
 */
LocalServer::~LocalServer()
{
  server->close();
  for(int i = 0; i < clients.size(); ++i)
  {
    clients[i]->close();
  }
}

/**
 * -----------------------
 * QThread requred methods
 * -----------------------
 */

/**
 * @brief run
 *  Initiate the thread.
 */
void LocalServer::run()
{
  server = new QLocalServer();

  QObject::connect(server, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));
  QObject::connect(this, SIGNAL(privateDataReceived(QString)), this, SLOT(slotOnData(QString)));

#ifdef Q_OS_UNIX
  // Make sure the temp address file is deleted
  QFile address(QString("/tmp/" LOCAL_SERVER_NAME));
  if(address.exists()){
    address.remove();
  }
#endif

  QString serverName = QString(LOCAL_SERVER_NAME);
  server->listen(serverName);
  while(server->isListening() == false){
    server->listen(serverName);
    msleep(100);
  }
  exec();
}

/**
 * @brief LocalServer::exec
 *  Keeps the thread alive. Waits for incomming connections
 */
void LocalServer::exec()
{
  while(server->isListening())
  {
    msleep(100);
    server->waitForNewConnection(100);
    for(int i = 0; i < clients.size(); ++i)
    {
      if(clients[i]->waitForReadyRead(100)){
        QByteArray data = clients[i]->readAll();
        emit privateDataReceived(data);
      }
    }
  }
}

/**
 * -------
 * SLOTS
 * -------
 */

/**
 * @brief LocalServer::slotNewConnection
 *  Executed when a new connection is available
 */
void LocalServer::slotNewConnection()
{
  clients.push_front(server->nextPendingConnection());
}

/**
 * @brief LocalServer::slotOnData
 *  Executed when data is received
 * @param data
 */
void LocalServer::slotOnData(QString data)
{
  if(data.contains("CMD:", Qt::CaseInsensitive)){
    onSGC(data);
  } else {
    emit dataReceived(data);
  }
}

/**
 * -------
 * Helper methods
 * -------
 */

void LocalServer::onCMD(QString data)
{
  //  Trim the leading part from the command
  data.replace(0, 4, "");

  QStringList commands;
  commands << "showUp";

  switch(commands.indexOf(data)){
    case 0:
      emit showUp();
  }
}

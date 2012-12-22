#ifndef APPLICATION_H
#define APPLICATION_H

#include "localserver.h"

#include <QGuiApplication>
#include <QLocalSocket>

/**
 * @brief The Application class handles trivial application initialization procedures
 */
class SingleApplication : public QGuiApplication
{
  Q_OBJECT
public:
  explicit SingleApplication(int, char *[]);
  ~SingleApplication();
  bool shouldContinue();

signals:
  void showUp();

private slots:
  void slotShowUp();
  
private:
  QLocalSocket* socket;
  LocalServer* server;
  bool _shouldContinue;

};

#endif // APPLICATION_H

#ifndef SINGLE_APPLICATION_H
#define SINGLE_APPLICATION_H

#include <QApplication>

class SingleApplicationPrivate;

/**
 * @brief The SingleApplication class handles multipe instances of the same Application
 * @see QApplication
 */
class SingleApplication : public QApplication
{
  Q_OBJECT
public:
  explicit SingleApplication(int&, char *[]);
  ~SingleApplication();

signals:
  void showUp();

private slots:
  void slotConnectionEstablished();

private:
  SingleApplicationPrivate *d_ptr;
};

#endif // SINGLE_APPLICATION_H

#ifndef SINGLE_APPLICATION_H
#define SINGLE_APPLICATION_H

#include <QtCore/QtGlobal>

#ifndef QAPPLICATION_CLASS
  #define QAPPLICATION_CLASS QApplication
#endif

#include QT_STRINGIFY(QAPPLICATION_CLASS)

class SingleApplicationPrivate;

/**
 * @brief The SingleApplication class handles multipe instances of the same Application
 * @see QApplication
 */
class SingleApplication : public QAPPLICATION_CLASS
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SingleApplication)

    typedef QAPPLICATION_CLASS app_t;

public:
    explicit SingleApplication( int &argc, char *argv[], uint8_t secondaryInstances = 0 );
    ~SingleApplication();

    bool isPrimary();
    bool isSecondary();

Q_SIGNALS:
    void showUp();

private Q_SLOTS:
    void slotConnectionEstablished();

private:
    SingleApplicationPrivate *d_ptr;
};

#endif // SINGLE_APPLICATION_H

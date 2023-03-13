
#include <QWidget>

#include "singleapplication.h"

#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif

void raiseWidget(QWidget* widget);

int main(int argc, char *argv[]) {

#ifdef Q_OS_WINDOWS
    SingleApplication app(argc, argv, true);

    if (app.isSecondary()) {

        AllowSetForegroundWindow( DWORD( app.primaryPid() ) );

        app.sendMessage("RAISE_WIDGET");

        return 0;
    }
#else
    SingleApplication app(argc, argv);
#endif

    QWidget* widget = new QWidget;

#ifdef Q_OS_WINDOWS
    QObject::connect(&app, &SingleApplication::receivedMessage,
                     widget, [widget] () { raiseWidget(widget); } );
#else
    QObject::connect(&app, &SingleApplication::instanceStarted,
                     widget, [widget] () { raiseWidget(widget); } );
#endif

    widget->show();

    return app.exec();
}

void raiseWidget(QWidget* widget) {
#ifdef Q_OS_WINDOWS
    HWND hwnd = (HWND)widget->winId();

    // check if widget is minimized to Windows task bar
    if (::IsIconic(hwnd)) {
        ::ShowWindow(hwnd, SW_RESTORE);
    }

    ::SetForegroundWindow(hwnd);
#else
    widget->show();
    widget->raise();
    widget->activateWindow();
#endif
}

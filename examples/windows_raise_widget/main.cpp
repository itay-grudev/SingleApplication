
#include <Windows.h>

#include <QWidget>

#include "singleapplication.h"

void RaiseWidget(QWidget* pWidget);

int main(int argc, char *argv[]) {

    SingleApplication app(argc, argv, true);

    if (app.isSecondary()) {

        AllowSetForegroundWindow( DWORD( app.primaryPid() ) );

        app.sendMessage("SHOW_WINDOW");

        return 0;
    }

    QWidget* widget = new QWidget;

    QObject::connect(&app, &SingleApplication::receivedMessage,
                     widget, [widget] () { RaiseWidget(widget); } );
    widget->show();

    return app.exec();
}

void RaiseWidget(QWidget* widget) {

    HWND hwnd = (HWND)widget->winId();

    if (::IsIconic(hwnd)) {
        ::ShowWindow(hwnd, SW_RESTORE);
    }

    ::SetForegroundWindow(hwnd);
}

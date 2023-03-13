# Single Application implementation
include(../../singleapplication.pri)
DEFINES += QAPPLICATION_CLASS=QApplication

QT += widgets
SOURCES += main.cpp
LIBS += User32.lib

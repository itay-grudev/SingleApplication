# Single Application implementation
include(../../singleapplication.pri)
DEFINES += QAPPLICATION_CLASS=QApplication

QT += widgets
SOURCES += main.cpp

win32{
  LIBS += User32.lib
}

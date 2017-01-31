QT += core network
CONFIG += c++11

HEADERS += $$PWD/singleapplication.h \
    $$PWD/singleapplication_p.h
SOURCES += $$PWD/singleapplication.cpp

INCLUDEPATH += $$PWD

win32 {
    LIBS += Advapi32.lib
}

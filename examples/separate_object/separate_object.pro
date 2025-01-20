# Single Application implementation
include(../../singleapplication.pri)
DEFINES += QAPPLICATION_CLASS=FreeStandingSingleApplication

SOURCES += main.cpp \
    messagereceiver.cpp

HEADERS += \
    messagereceiver.h

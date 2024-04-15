
TEMPLATE = app
TARGET = tst_remote

QT = core testlib remote network
CONFIG += testcase

SOURCES += tst_remote.cpp
HEADERS += \
    dummy-server.h \
    dummy-client.h

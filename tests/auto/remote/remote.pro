
TEMPLATE = app
TARGET = tst_remote

QT = core testlib remote network
QT += testlib-private
CONFIG += testcase
CONFIG += exceptions

SOURCES += \
    tst_remote.cpp \
    message-storage.cpp \
    # List terminator.

HEADERS += \
    dummy-server.h \
    dummy-client.h \
    message-storage.h \
    # List terminator.

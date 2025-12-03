TEMPLATE = app
TARGET = remote-helloClient

#QT -= gui
QT += network
QT += remote

CONFIG += console exceptions

DEFINES += QT_SHAREDPOINTER_TRACK_POINTERS

SOURCES = \
    $$PWD/src/main-client.cpp \
    $$PWD/src/client.cpp

HEADERS = \
    $$PWD/src/client.h

# WARNING: below files don't exist unless server is already built first. and
# that's why `../hello.pro` has `CONFIG += ordered`, also
# moc auto-include's below into server's compile, but
# for client, it's your responsibility to find them.
SOURCES += $$PWD/../helloServer/src/helloservice_remote.cpp
HEADERS += $$PWD/../helloServer/src/helloservice_remote.h

target.path = $$[QT_INSTALL_EXAMPLES]/remote/hello-example/helloClient
INSTALLS += target

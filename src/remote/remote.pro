#
# QRemote (remote-procedure-call support for QObject).
#
TEMPLATE = lib

TARGET = QtRemote
MODULE = remote
MODULE_CONFIG = moc resources remote

CONFIG *= $$MODULE_CONFIG
DEFINES += $$MODULE_DEFINES

QT += core-private
CONFIG += exceptions

DEFINES += QT_REMOTE_BUILDING

CONFIG(debug, debug|release) {
    DEFINES += QT_SHAREDPOINTER_TRACK_POINTERS
}


load(extras)
SOURCES += "$$extras/tools/customtypes.cpp"

HEADERS *= \
    $$files($$PWD/src/*.h, true) \
    # List termination comment.

SOURCES *= \
    $$files($$PWD/src/*.cpp, true) \
    # List termination comment.

load(qt_module)

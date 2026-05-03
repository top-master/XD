#
# QRemote (remote-procedure-call support for QObject).
#
TEMPLATE = lib

TARGET = QtRemote
MODULE = remote
MODULE_CONFIG = moc resources remote

CONFIG *= $$MODULE_CONFIG
DEFINES += $$MODULE_DEFINES

# `mkspecs/features/spec_pre.prf` defaults `QT` to `core gui`. Remote does not
# use any QtGui types, and on builds where Gui isn't compiled (e.g. XD-mini's
# headless path) the default would error with "Unknown module(s) in QT: gui".
QT = core core-private
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

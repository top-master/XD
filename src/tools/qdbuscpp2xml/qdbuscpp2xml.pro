option(host_build)
# TRACE/tools cross-compile: link to cross-target's bootstrap lib #2.
CONFIG += force_bootstrap
QT = core-private
force_bootstrap: QT += bootstrap_dbus-private
else: QT += dbus-private
DEFINES += QT_NO_CAST_FROM_ASCII
QMAKE_CXXFLAGS += $$QT_HOST_CFLAGS_DBUS

include(../moc/moc.pri)

SOURCES += qdbuscpp2xml.cpp

load(qt_tool)

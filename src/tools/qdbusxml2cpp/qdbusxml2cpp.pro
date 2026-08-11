option(host_build)
# TRACE/tools cross-compile: link to cross-target's bootstrap lib #1,
# instead of host's Qt libs -- so `CONFIG += force_bootstrap` below picks the
# static bootstrap corelib and (for QtDBus) bootstrap_dbus-private, not the
# native libQt5Core.so or dbus-private. A cross build never produces a host Qt,
# so without the flag the tool links whatever is in lib/, an incompatible
# native build (wrong compiler, ABI mismatch).
CONFIG += force_bootstrap
QT = core-private
force_bootstrap: QT += bootstrap_dbus-private
else: QT += dbus-private
DEFINES += QT_NO_CAST_FROM_ASCII
QMAKE_CXXFLAGS += $$QT_HOST_CFLAGS_DBUS

SOURCES = qdbusxml2cpp.cpp

load(qt_tool)

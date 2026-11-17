# libdbus-1, the D-Bus client library, from the optional `src/3rdparty/dbus`
# submodule (dbus 1.16.2), built for a --memory-safe build only: QtDBus loads
# libdbus-1 at run time (see qdbus_symbols.cpp), and a memory-safe process
# loads only a library built the same way, never the host's.
#
# config.h and dbus/dbus-arch-deps.h come from dbus's own CMake configure, and
# libdbus.version is its linker version script, which also exports Fil-C's
# names of the functions (`pizlonated_dbus_*`); hence QtDBus finds them.

TEMPLATE = lib
TARGET = dbus-1
VERSION = 3.38.3
CONFIG -= qt
CONFIG += warn_off
DESTDIR = $$[QT_INSTALL_LIBS]

DBUS_SOURCE = $$PWD/../dbus
INCLUDEPATH += $$PWD $$DBUS_SOURCE
DEFINES += DBUS_COMPILATION HAVE_CONFIG_H dbus_1_EXPORTS
QMAKE_CFLAGS += -fno-common
QMAKE_LFLAGS += -Wl,--version-script=$$PWD/libdbus.version

SOURCES += \
    $$DBUS_SOURCE/dbus/dbus-address.c \
    $$DBUS_SOURCE/dbus/dbus-auth.c \
    $$DBUS_SOURCE/dbus/dbus-bus.c \
    $$DBUS_SOURCE/dbus/dbus-connection.c \
    $$DBUS_SOURCE/dbus/dbus-credentials.c \
    $$DBUS_SOURCE/dbus/dbus-dataslot.c \
    $$DBUS_SOURCE/dbus/dbus-errors.c \
    $$DBUS_SOURCE/dbus/dbus-file.c \
    $$DBUS_SOURCE/dbus/dbus-file-unix.c \
    $$DBUS_SOURCE/dbus/dbus-hash.c \
    $$DBUS_SOURCE/dbus/dbus-internals.c \
    $$DBUS_SOURCE/dbus/dbus-keyring.c \
    $$DBUS_SOURCE/dbus/dbus-list.c \
    $$DBUS_SOURCE/dbus/dbus-marshal-basic.c \
    $$DBUS_SOURCE/dbus/dbus-marshal-byteswap.c \
    $$DBUS_SOURCE/dbus/dbus-marshal-header.c \
    $$DBUS_SOURCE/dbus/dbus-marshal-recursive.c \
    $$DBUS_SOURCE/dbus/dbus-marshal-validate.c \
    $$DBUS_SOURCE/dbus/dbus-memory.c \
    $$DBUS_SOURCE/dbus/dbus-mempool.c \
    $$DBUS_SOURCE/dbus/dbus-message.c \
    $$DBUS_SOURCE/dbus/dbus-misc.c \
    $$DBUS_SOURCE/dbus/dbus-nonce.c \
    $$DBUS_SOURCE/dbus/dbus-object-tree.c \
    $$DBUS_SOURCE/dbus/dbus-pending-call.c \
    $$DBUS_SOURCE/dbus/dbus-pipe.c \
    $$DBUS_SOURCE/dbus/dbus-pipe-unix.c \
    $$DBUS_SOURCE/dbus/dbus-resources.c \
    $$DBUS_SOURCE/dbus/dbus-server.c \
    $$DBUS_SOURCE/dbus/dbus-server-debug-pipe.c \
    $$DBUS_SOURCE/dbus/dbus-server-socket.c \
    $$DBUS_SOURCE/dbus/dbus-server-unix.c \
    $$DBUS_SOURCE/dbus/dbus-sha.c \
    $$DBUS_SOURCE/dbus/dbus-signature.c \
    $$DBUS_SOURCE/dbus/dbus-string.c \
    $$DBUS_SOURCE/dbus/dbus-syntax.c \
    $$DBUS_SOURCE/dbus/dbus-sysdeps.c \
    $$DBUS_SOURCE/dbus/dbus-sysdeps-pthread.c \
    $$DBUS_SOURCE/dbus/dbus-sysdeps-unix.c \
    $$DBUS_SOURCE/dbus/dbus-threads.c \
    $$DBUS_SOURCE/dbus/dbus-timeout.c \
    $$DBUS_SOURCE/dbus/dbus-transport.c \
    $$DBUS_SOURCE/dbus/dbus-transport-socket.c \
    $$DBUS_SOURCE/dbus/dbus-transport-unix.c \
    $$DBUS_SOURCE/dbus/dbus-userdb.c \
    $$DBUS_SOURCE/dbus/dbus-uuidgen.c \
    $$DBUS_SOURCE/dbus/dbus-watch.c

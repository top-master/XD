#
# Similar to XD-mini.pro file, however, builds with qt_static mode enabled.
#

TEMPLATE = subdirs
CONFIG += ordered
# Ensures we don't depend on Qt-modules until they're built.
unset(QT)

# Note: we could exclude `bootstrap`, `moc` and `uic` from the below list if
# our `XD.pro` is already built for shared usage, but let's keep those updated.
SUBDIRS += \
    $$PWD/src/3rdparty/pcre \
    $$PWD/src/tools/bootstrap \

win32: SUBDIRS += $$PWD/src/tools/idc

SUBDIRS += \
    $$PWD/src/tools/moc \
    $$PWD/src/tools/uic \
    $$PWD/src/tools/rcc \
    $$PWD/src/corelib \

win32: SUBDIRS += $$PWD/src/winmain

SUBDIRS += \
    $$PWD/src/dbus \
    $$PWD/src/network \
    $$PWD/src/xml \
    $$PWD/src/3rdparty/harfbuzz-ng \
    $$PWD/src/3rdparty/freetype \
    $$PWD/src/gui \
    $$PWD/src/platformsupport \
    $$PWD/src/widgets \
    $$PWD/src/printsupport \
    $$PWD/src/remote \
    $$PWD/src/testlib

# Enables global qt_static setting (if not already).
if ( ! exists($$OUT_PWD/.qmake.cache)) {
    CONTENT = "CONFIG += qt_static"
    write_file($$OUT_PWD/.qmake.cache, CONTENT)|error("Failed to write global-settings.")
} else: if ( ! qt_static) {
    error("Invalid global-settings file, if it\'s not needed, delete the file at: $$OUT_PWD/.qmake.cache")
}

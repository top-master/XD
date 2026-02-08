#
# See `./README.md` for few details.
#
# This file is normally loaded by the platform-specific file, like by
# the `features/win32/qt_config.prf` file, which loads
# the `$$PWD/modules/qt_*.pri` modules right after this file.
#

# TRACE/mkspecs note: By default `release` is added to `CONFIG`,
# but with the command-line `CONFIG+=debug`, `release` will be considered removed,
# basically, the last in list overrides any previous, meaning,
# any project's `.pro` file can override said command-line (to force always `release`).
CONFIG += release shared rtti no_plugin_manifest directwrite qpa
CONFIG += compat_verbose
host_build {
    QT_ARCH = i386
    QT_TARGET_ARCH = i386
} else {
    QT_ARCH = i386
}
QT_CONFIG += \
    minimal-config small-config medium-config large-config full-config \
    debug_and_release debug release shared build_all \
    zlib dynamicgl \
    png \
    freetype harfbuzz accessibility \
    opengl \
    ssl openssl \
    dbus \
    audio-backend \
    directwrite \
    native-gestures qpa concurrent

## SQL support can be disabled by commenting:
QT_CONFIG += sql

## Qt would disable text-codecs for Windows, XD keeps it always if possible.
QT_CONFIG += codecs

# TRACE/gui: disables OpenGL usages for Windows, except if either:
# its included into the compiler's normally used Windows-SDK,
# or, DirectX-SDK is installed.
#
# Sync below DirectX checks with: src/angle/src/common/common.pri
#
win32: !if(force_angle | force_opengl) {
    winrt | if(msvc: greaterThan(MSC_VER, 1699)) {
        # DirectX is included in the Windows 8 Kit -- nothing to disable.
    } else {
        TMP = $$(DXSDK_DIR)
        isEmpty(TMP) {
            QT_CONFIG -= angle dynamicgl opengl
        }
    }
}

# Improves auto-tests if debug-build.
!CONFIG(debug, release|debug) {
    DEFINES += QT_BUILD_INTERNAL
    QT_CONFIG += private_tests
}

# Versioning.
QT_VERSION = 5.6.5
QT_MAJOR_VERSION = $$section(QT_VERSION, ., 0, 0)
QT_MINOR_VERSION = $$section(QT_VERSION, ., 1, 1)
QT_PATCH_VERSION = $$section(QT_VERSION, ., 2, 2)

QT_EDITION = OpenSource
win32: QT_LICHECK = licheck.exe
else:  QT_LICHECK =
QT_RELEASE_DATE = 2017-09-17
QT_CL_MAJOR_VERSION = 19
QT_CL_MINOR_VERSION = 00
QT_CL_PATCH_VERSION = 24215

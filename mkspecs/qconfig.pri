CONFIG+= release shared rtti no_plugin_manifest directwrite qpa
host_build {
    QT_ARCH = i386
    QT_TARGET_ARCH = i386
} else {
    QT_ARCH = i386
}
QT_CONFIG += \
    minimal-config small-config medium-config large-config full-config \
    debug_and_release build_all debug release shared build_all \
    zlib dynamicgl \
    png \
    freetype harfbuzz accessibility \
    opengl \
    ssl openssl \
    dbus \
    audio-backend \
    directwrite \
    native-gestures qpa concurrent

## SQL support is disabled by default, enable by un-commenting:
#QT_CONFIG += sql

#versioning 
QT_VERSION = 5.6.3
QT_MAJOR_VERSION = 5
QT_MINOR_VERSION = 6
QT_PATCH_VERSION = 3

QT_EDITION = OpenSource
QT_LICHECK = licheck.exe
QT_RELEASE_DATE = 2017-09-17
QT_CL_MAJOR_VERSION = 19
QT_CL_MINOR_VERSION = 00
QT_CL_PATCH_VERSION = 24215

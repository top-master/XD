#!/bin/bash

# License: Apache 2.0 without attribution need.

cd "${0%[/\\]*}" > /dev/null 2>&1
ROOT=$(pwd)
QT_DIR="$(dirname "$ROOT")"

BH_ROOT=$QT_DIR
BH_SCRIPT_NAME=$(basename "$0")
BH_BUILD_DIR_SUFFIX=/qmake
. "$QT_DIR/tools/build-handler.sh"

QT_VERSION=$(awk -F'[= ]+' '/^[[:space:]]*QT_VERSION[[:space:]]*=/{print $2; exit}' "$QT_DIR/mkspecs/qconfig.pri")
[ -n "$QT_VERSION" ] || bh_error "could not read QT_VERSION from $QT_DIR/mkspecs/qconfig.pri"
QT_MAJOR_VERSION=${QT_VERSION%%.*}
_rest=${QT_VERSION#*.}
QT_MINOR_VERSION=${_rest%%.*}
QT_PATCH_VERSION=${_rest#*.}
QT_HEADER_VERSION=$(awk -F'[= ]+' '/^[[:space:]]*MODULE_HEADER_VERSION[[:space:]]*=/{print $2; exit}' "$QT_DIR/.qmake.conf")
: "${QT_HEADER_VERSION:=$QT_VERSION}"

HOST_OS=$(uname -s)
MAC_OBJS=
MAC_SRCS=
MAC_LFLAGS=
MAC_CXXFLAGS=
case "$HOST_OS" in
    Darwin)
        : "${QMAKESPEC:=macx-clang}"
        : "${CXX:=clang++}"
        MAC_OBJS="qsettings_mac.o qcore_mac.o qcore_mac_objc.o qstring_mac.o"
        MAC_SRCS="$QT_DIR/src/corelib/io/qsettings_mac.cpp \
$QT_DIR/src/corelib/kernel/qcore_mac.cpp \
$QT_DIR/src/corelib/kernel/qcore_mac_objc.mm \
$QT_DIR/src/corelib/tools/qstring_mac.mm"
        MAC_LFLAGS="-framework Foundation -framework CoreServices -framework ApplicationServices"
        MAC_CXXFLAGS="-fconstant-cfstrings -Wno-deprecated-register"
        ;;
    Linux)
        : "${QMAKESPEC:=linux-g++}"
        : "${CXX:=g++}"
        ;;
    *)
        bh_error "unsupported host OS: $HOST_OS"
        ;;
esac
command -v "$CXX" >/dev/null 2>&1 \
    || bh_error "C++ compiler '$CXX' not found in PATH (install build-essential / gcc-c++)."

QTOBJS="qfilesystemengine_unix.o qfilesystemiterator_unix.o qfsfileengine_unix.o qlocale_unix.o $MAC_OBJS"
QTSRCS="$QT_DIR/src/corelib/io/qfilesystemengine_unix.cpp \
$QT_DIR/src/corelib/io/qfilesystemiterator_unix.cpp \
$QT_DIR/src/corelib/io/qfsfileengine_unix.cpp \
$QT_DIR/src/corelib/tools/qlocale_unix.cpp $MAC_SRCS"

drive_qmake_build() {
    case "$BH_MODE" in
        release) MODE_CXXFLAGS="-std=c++14 -O2" ;;
        debug)   MODE_CXXFLAGS="-std=c++14 -O0 -g" ;;
    esac
    ARCH_CXXFLAGS=
    ARCH_LFLAGS=
    if [ "$HOST_OS" = Darwin ]; then
        ARCH_CXXFLAGS="-arch $BH_CURRENT_ARCH"
        ARCH_LFLAGS="-arch $BH_CURRENT_ARCH"
    fi
    bh_make_step make "" "$JOBS" -f "$QT_DIR/qmake/Makefile.unix" \
        SOURCE_PATH="$QT_DIR" \
        BUILD_PATH="$QT_DIR" \
        INC_PATH="$QT_DIR/include" \
        QMAKESPEC="$QT_DIR/mkspecs/$QMAKESPEC" \
        QT_VERSION="$QT_VERSION" \
        QT_MAJOR_VERSION="$QT_MAJOR_VERSION" \
        QT_MINOR_VERSION="$QT_MINOR_VERSION" \
        QT_PATCH_VERSION="$QT_PATCH_VERSION" \
        QT_HEADER_VERSION="$QT_HEADER_VERSION" \
        CXX="$CXX" \
        EXEEXT="" \
        RM_F="rm -f" \
        RM_RF="rm -rf" \
        LFLAGS="$MAC_LFLAGS $ARCH_LFLAGS" \
        EXTRA_CXXFLAGS="$MODE_CXXFLAGS $MAC_CXXFLAGS $ARCH_CXXFLAGS -DPROJECT_FOLDER=\"\\\"$ROOT/\\\"\" -DQT_NO_EXCEPTIONS" \
        QTOBJS="$QTOBJS" \
        QTSRCS="$QTSRCS"
}

bh_template_makefile drive_qmake_build "$QT_DIR/bin/qmake" "$@"

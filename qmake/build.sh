#!/bin/bash

# License: Apache 2.0 without attribution need.

# MARK: Common.

# Gets the absolute path of this script's directory.
cd "${0%[/\\]*}" > /dev/null 2>&1;
ROOT=$(pwd)

error() {
    echo "$ROOT/${0##*/}:${2:-1}:1: error: $1" 1>&2;
    exit 1
}

# Gets QtBase's directory, one level up from ROOT.
QT_DIR="$(dirname "$ROOT")"


# Command-line argument parsing.
#

mode=release
while [ $# -gt 0 ]; do
    case "$1" in
    -v|--verbose) verbose=1; shift;;
    -c|--clean) cleanBuild=1; shift;;
    --release) mode=release; shift;;
    --debug)   mode=debug;   shift;;
    -d|--directory)
        # `-d` requires a non-flag argument.
        if [ $# -gt 1 ]; then
            case "$2" in
                -*) error "-d requires value." $LINENO;;
                *)  export QMAKE_BUILD_DIR=$2; shift 2;;
            esac
        else
            error "-d requires value." $LINENO
        fi ;;
    --)
        while [ $# -gt 0 ]; do BACKUP="$BACKUP;$1"; shift; done
        break;;
    *)
        BACKUP="$BACKUP;$1"
        shift
        ;;
    esac
done
# Restore unused arguments.
while [ -n "$BACKUP" ] ; do
    [ -n "${BACKUP%%;*}" ] && set -- "$@" "${BACKUP%%;*}"
    # Stop once no `;` remains in BACKUP. POSIX-portable form of the bash
    # `[ "$BACKUP" = "${BACKUP/;/}" ]` test.
    case "$BACKUP" in
        *\;*) ;;
        *)    break;;
    esac
    BACKUP="${BACKUP#*;}"
done

# Force error handling. `pipefail` isn't in POSIX 2008 (it landed in Issue 8 /
# 2024), so try to enable it but tolerate shells that don't know it.
set -e
(set -o pipefail) 2>/dev/null && set -o pipefail || true

: "${QMAKE_BUILD_DIR:=$(dirname "$QT_DIR")/build/$(basename "$QT_DIR")-$mode/qmake}"

# Optionally, remove previous build.
echo "Build directory:"
echo "$QMAKE_BUILD_DIR"
if [ "${cleanBuild:-0}" -gt 0 ]; then
    rm -rf "${QMAKE_BUILD_DIR}"
fi
mkdir -p "$QMAKE_BUILD_DIR"

# Detect host. Mac is a Unix, so it goes through Makefile.unix; we just
# default to a different mkspec/compiler and append the Mac-only sources,
# objects, frameworks, and -fconstant-cfstrings to the per-Unix lists below.
# Keeping the Mac-aware bits here (instead of in Makefile.unix) avoids
# pulling in $(shell)/$(filter)/ifeq -- those are GNU-only directives.
HOST_OS=$(uname -s)
MAC_OBJS=
MAC_SRCS=
MAC_LFLAGS=
MAC_CXXFLAGS=
HOST_CXXFLAGS="-Wno-deprecated-register"
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
        MAC_CXXFLAGS="-fconstant-cfstrings"
        ;;
    *)
        : "${QMAKESPEC:=linux-g++}"
        : "${CXX:=g++}"
        ;;
esac

# Ensures Compiler is installed on this environment.
command -v "$CXX" >/dev/null 2>&1 \
    || error "C++ compiler '$CXX' not found in PATH (install build-essential / gcc-c++)." $LINENO

# Read QT_VERSION (e.g. 5.6.6) from the canonical place.
QT_VERSION=$(awk -F'[= ]+' '/^[[:space:]]*QT_VERSION[[:space:]]*=/{print $2; exit}' "$QT_DIR/mkspecs/qconfig.pri")
[ -n "$QT_VERSION" ] || error "could not read QT_VERSION from $QT_DIR/mkspecs/qconfig.pri" $LINENO
QT_MAJOR_VERSION=${QT_VERSION%%.*}
_rest=${QT_VERSION#*.}
QT_MINOR_VERSION=${_rest%%.*}
QT_PATCH_VERSION=${_rest#*.}

# Header version may differ from runtime QT_VERSION when MODULE_HEADER_VERSION
# in .qmake.conf is pinned (see XD's .qmake.conf). Falls back to QT_VERSION.
QT_HEADER_VERSION=$(awk -F'[= ]+' '/^[[:space:]]*MODULE_HEADER_VERSION[[:space:]]*=/{print $2; exit}' "$QT_DIR/.qmake.conf")
: "${QT_HEADER_VERSION:=$QT_VERSION}"

# Per-Unix common extras + (empty on non-Mac) Mac extras. Makefile.unix stays
# free of any platform conditionals -- it just consumes whatever shows up here.
QTOBJS="qfilesystemengine_unix.o qfilesystemiterator_unix.o qfsfileengine_unix.o qlocale_unix.o $MAC_OBJS"
QTSRCS="$QT_DIR/src/corelib/io/qfilesystemengine_unix.cpp \
$QT_DIR/src/corelib/io/qfilesystemiterator_unix.cpp \
$QT_DIR/src/corelib/io/qfsfileengine_unix.cpp \
$QT_DIR/src/corelib/tools/qlocale_unix.cpp $MAC_SRCS"

case "$mode" in
    release) MODE_CXXFLAGS="-O2" ;;
    debug)   MODE_CXXFLAGS="-O0 -g" ;;
esac

# Build out-of-tree: cwd = $QMAKE_BUILD_DIR (under git-ignored tmp/), so .o
# files land there and qmake/ source dir stays clean. The Makefile is invoked
# by absolute path; SOURCE_PATH/BUILD_PATH/INC_PATH are absolute so the
# recipes don't depend on cwd. The link rule writes to $(BUILD_PATH)/bin/qmake,
# which resolves to $QT_DIR/bin/qmake regardless of where we run from.
cd "$QMAKE_BUILD_DIR"

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
LFLAGS="$MAC_LFLAGS" \
EXTRA_CXXFLAGS="$MODE_CXXFLAGS $MAC_CXXFLAGS $HOST_CXXFLAGS -DPROJECT_FOLDER=\"\\\"$ROOT/\\\"\" -DQT_NO_EXCEPTIONS" \
QTOBJS="$QTOBJS" \
QTSRCS="$QTSRCS" \
exec make -f "$QT_DIR/qmake/Makefile.unix"

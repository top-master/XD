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

while [[ $# -gt 0 ]]; do
    case "$1" in
    -v|--verbose) verbose=1; shift;;
    -c|--clean) cleanBuild=1; shift;;
    -d|--directory)
        if [[ $# -gt 1 && "$2" != -* ]]; then
            export QMAKE_BUILD_DIR=$2; shift 2
        else
            error "-d requires value." $LINENO
        fi ;;
    --)
        while [[ $# -gt 0 ]]; do BACKUP="$BACKUP;$1"; shift; done
        break;;
    *)
        BACKUP="$BACKUP;$1"
        shift
        ;;
    esac
done
# Restore unused arguments.
while [ -n "$BACKUP" ] ; do
    [ ! -z "${BACKUP%%;*}" ] && set -- "$@" "${BACKUP%%;*}"
    [ "$BACKUP" = "${BACKUP/;/}" ] && break
    BACKUP="${BACKUP#*;}"
done

# Force error handling.
set -e
set -o pipefail

# Optionally, remove previous build.
echo "Build directory:"
echo "$QMAKE_BUILD_DIR"
if [[ $cleanBuild -gt 0 ]]; then
    rm -rf "${QMAKE_BUILD_DIR}"
fi

# Runs make with needed environment variables and our custom Makefile name.
#
# Unused env-vars: EXTRA_CPPFLAGS
SOURCE_PATH="$QT_DIR" make -f Makefile.unix

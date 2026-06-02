#!/bin/sh

# License: Apache 2.0 without attribution need.

# MARK: Common.

# Gets the absolute path of this script's directory.
cd "${0%[/\\]*}" > /dev/null 2>&1
ROOT=$(pwd)

# MARK: Hand off to the subdirs template driver.
#
# XD *is* the framework, so XD_DIR points to this very repo
# (bh_template_subdirs needs XD_DIR set; for self-builds we point
# it at $ROOT). BH_TEST_ROOT plugs the `--test` shape: XD's tests
# live under `tests/auto/auto.pro` (a TEMPLATE = subdirs of
# testcase apps), which `bh_run_tests --subdirs` knows how to drive.
#
# Everything else (arg parsing, build dir, license gate, lifecycle
# wipes, qmake bootstrap, qmake, make, plugin staging, tests, run)
# lives in `tools/build-handler.sh`. The only XD-specific overrides
# happen above the source line below.

XD_DIR=$ROOT
BH_ROOT=$ROOT
BH_SCRIPT_NAME=build.sh
BH_TEST_ROOT=$ROOT/tests/auto/auto.pro
. "$ROOT/tools/build-handler.sh"

# MARK: --static: XD-only flag.
#
# Builds the static-link subset (`XD-static.pro` selects the
# qt_static-friendly subprojects) into its own build dir
# (`build/XD-static`) so the static tree doesn't stomp the
# regular `build/XD`. Implemented entirely in this script:
#   * `BH_BUILD_DIR_SUFFIX=-static` is the documented hook
#     bh_template_subdirs reads when composing $BUILD_DIR, so no
#     tools/build-handler.sh change is needed.
#   * `--static` is consumed here and stripped from the
#     forwarded argv -- otherwise bh_parse_args would treat it
#     as an unknown positional and fold it into the qmake
#     remaining-args.
_xd_pro=$ROOT/XD-mini.pro
_xd_static=0
# Consumes `--static` and forwards the rest to bh_template_subdirs.
#
# We rotate the args through the positional parameters (pop the front with
# `shift`, push keepers to the back) rather than joining them into one
# string, so the forwarded `"$@"` stays quoted end-to-end. That keeps a
# glob argument such as `--test '**/*qmake*'` literal (an unquoted,
# space-joined string would word-split it and pathname-expand it here,
# before build-handler.sh could resolve it) and likewise survives args
# that contain spaces.
_xd_argc=$#
while [ "$_xd_argc" -gt 0 ]; do
    _xd_arg=$1
    shift
    _xd_argc=$((_xd_argc - 1))
    case "$_xd_arg" in
        --static)
            _xd_pro=$ROOT/XD-static.pro
            BH_BUILD_DIR_SUFFIX=-static
            _xd_static=1
            ;;
        *)
            set -- "$@" "$_xd_arg"
            ;;
    esac
done

# Seed `.qmake.cache` BEFORE qmake's `-r` recursive walk -- writing it
# from XD-static.pro's body lands too late (qmake has already started
# reading subdir .pro files in-process by then, so `CONFIG += qt_static`
# never makes it into corelib/gui/widgets/etc., and the per-module
# Makefile.Release ends up targeting the shared dylib instead of the
# static .a archive).
if [ "$_xd_static" -eq 1 ]; then
    : "${BUILD_DIR:=$(dirname "$BH_ROOT")/build/$(basename "$BH_ROOT")${BH_BUILD_DIR_SUFFIX:-}}"
    mkdir -p "$BUILD_DIR"
    printf 'CONFIG += qt_static\n' > "$BUILD_DIR/.qmake.cache"
    export BUILD_DIR

    # The static module .pri files are auto-generated (git-ignored) and read
    # by consumers (e.g. a consuming app's qt_static build), so a normal
    # rebuild must keep them. Hand them to bh_clean as extras, so that
    # `--clean` can simulate an out-of-the-box state
    # (without a real git-re-clone, which would drop the OpenSSL submodule).
    export BH_CLEAN_EXTRA="$ROOT/mkspecs/modules/static/qt_lib_*.pri"
fi

bh_template_subdirs "$_xd_pro" "$@"

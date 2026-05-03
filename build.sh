#!/bin/sh

# License: Apache 2.0 without attribution need.

# MARK: Common.

# Gets the absolute path of this script's directory.
cd "${0%[/\\]*}" > /dev/null 2>&1
ROOT=$(pwd)

error() {
    echo "$ROOT/${0##*/}:${2:-1}:1: error: $1" 1>&2
    exit 1
}

# Force error handling. `pipefail` isn't in POSIX 2008 (it landed in Issue 8 /
# 2024), so try to enable it but tolerate shells that don't know it.
set -e
(set -o pipefail) 2>/dev/null && set -o pipefail || true

# MARK: Argument parsing.

verbose=0
headless=0
while [ $# -gt 0 ]; do
    case "$1" in
        -v|--verbose)  verbose=1; shift;;
        --headless)    headless=1; shift;;
        --)            shift; break;;
        *)             break;;
    esac
done

# MARK: Defaults.

# Parallel-job count for make. Override via the JOBS env var. Picks the host's
# logical CPU count using whichever of getconf/sysctl/nproc is present; falls
# back to 4 on shells that lack all three.
if [ -z "${JOBS:-}" ]; then
    JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null \
           || sysctl -n hw.logicalcpu 2>/dev/null \
           || nproc 2>/dev/null \
           || echo 4)
fi

# Pick the mkspec by host. macx-clang on Darwin, linux-g++ otherwise.
case "$(uname -s)" in
    Darwin) : "${QMAKESPEC:=macx-clang}";;
    *)      : "${QMAKESPEC:=linux-g++}";;
esac

# `mkspecs/features/xd_functions.prf` forbids building inside the source tree,
# so default to a sibling directory next to the source root.
: "${XD_BUILD_DIR:=$(dirname "$ROOT")/XD-build}"

# Show where we will build, and confirm unless --headless was passed.
echo "Build directory:"
echo "  $XD_BUILD_DIR"
if [ "$headless" -eq 0 ]; then
    printf 'Do you want to continue? [Y/n] '
    read -r _answer
    case "$_answer" in
        n|N|no|NO|No) echo "Aborted."; exit 1;;
    esac
fi

# Per-step logs go to disk (no risk of overflowing a shell variable for a long
# build). Each invocation is a fresh re-build, so wipe the prior logs up
# front -- only deletes existing `*.log` files; the folder itself is reused.
LOGS=$XD_BUILD_DIR/logs
mkdir -p "$LOGS"
rm -f "$LOGS"/*.log

# MARK: Run helper.

_step=0

# Run a named step. Stream goes to `$LOGS/<NN>-<name>.log`; verbose mode also
# tees it to the terminal. Non-verbose mode is silent on success and dumps the
# step's log to stderr only on failure (preserving the original exit status).
run() {
    _name=$1; shift
    _step=$((_step + 1))
    _log=$LOGS/$(printf '%02d-%s.log' "$_step" "$_name")
    if [ "$verbose" -gt 0 ]; then
        # `pipefail` (best-effort above) propagates the inner command's exit.
        "$@" 2>&1 | tee "$_log"
    else
        if "$@" >"$_log" 2>&1; then
            :
        else
            _status=$?
            cat "$_log" 1>&2
            exit "$_status"
        fi
    fi
}

# MARK: Build.

# Bootstrap bin/qmake on demand. We track bin/qmake in git, so a tracked
# binary skips this step entirely. The bootstrap's intermediate objects land
# in `$XD_BUILD_DIR/qmake/` (passed via `-d`), keeping the source tree clean.
if [ ! -x "$ROOT/bin/qmake" ]; then
    run bootstrap-qmake sh "$ROOT/qmake/build.sh" -d "$XD_BUILD_DIR/qmake"
fi

cd "$XD_BUILD_DIR"

run qmake "$ROOT/bin/qmake" -spec "$ROOT/mkspecs/$QMAKESPEC" "$ROOT/XD-mini.pro" "$@"
run make  make -j"$JOBS"

# License: Apache 2.0 without attribution need.
#
# --very-verbose helpers of `build-handler.sh`, which sources this file after
# its own helpers: the trace of what the qmake skip check of `bh_run_qmake`
# compared, with the files' modified-times.


# bh_qmake_trace <line>...
#
# Prints each <line> to stderr under --very-verbose (log level 3), after a
# `qmake check:` tag; such lines tell what `bh_run_qmake` compared to decide
# whether qmake runs. Prints nothing at a lower log level.
bh_qmake_trace() {
    [ "${BH_LOG_LEVEL:-1}" -ge 3 ] || return 0
    for _bqt_line in "$@"; do
        printf 'qmake check: %s\n' "$_bqt_line" 1>&2
    done
}

# bh_file_time <file>
#
# Prints the modified-time of <file> as `stat` tells it, to the nanosecond
# where the file system keeps it, or `missing` when there is no such file.
bh_file_time() {
    if [ ! -e "$1" ]; then
        echo missing
        return 0
    fi
    stat -c '%y' "$1" 2>/dev/null \
        || stat -f '%Sm' -t '%Y-%m-%d %H:%M:%S %z' "$1"
}

# bh_qmake_trace_inputs
#
# Traces (see `bh_qmake_trace`) what `bh_run_qmake` looks at first in the
# current folder: the `find` that runs, whether BH_ROOT is a link, the --qmake
# flag, its Makefile `$_bhq_makefile` with its modified-time, whether a build
# for another toolchain or variant made that Makefile, and the `.qmake.static`
# list with its modified-time.
bh_qmake_trace_inputs() {
    [ "${BH_LOG_LEVEL:-1}" -ge 3 ] || return 0
    _bqti_static=$BUILD_DIR/.qmake.static
    _bqti_static_newer=no
    if [ -f "$_bhq_makefile" ] && bh_static_list_newer "$_bhq_makefile"; then
        _bqti_static_newer=yes
    fi
    _bqti_root_link=no
    if [ -L "$BH_ROOT" ]; then
        _bqti_root_link="yes, to $(readlink "$BH_ROOT")"
    fi
    bh_qmake_trace "$_label, in $PWD:" \
        "  find: $(command -v find)" \
        "  $BH_ROOT is a link: $_bqti_root_link" \
        "  --qmake (BH_FORCE_QMAKE): ${BH_FORCE_QMAKE:-0}" \
        "  $PWD/$_bhq_makefile: $(bh_file_time "$_bhq_makefile")" \
        "  made for another toolchain or variant: $_bhq_build_differs" \
        "  $_bqti_static: $(bh_file_time "$_bqti_static")" \
        "  that list is newer than the Makefile: $_bqti_static_newer"
}

# bh_qmake_trace_newer <newer-file>
#
# Traces (see `bh_qmake_trace`) the result of `bh_newer_project_file` for the
# Makefile `$_bhq_makefile`: <newer-file> with its modified-time, or that none
# is newer, when <newer-file> is empty.
bh_qmake_trace_newer() {
    [ "${BH_LOG_LEVEL:-1}" -ge 3 ] || return 0
    _bqtn_kinds=".pro, .pri, or .prf"
    if [ "${BH_IGNORE_PRI:-0}" -ne 0 ]; then
        _bqtn_kinds=".pro"
    fi
    bh_qmake_trace "Looks under $BH_ROOT for a $_bqtn_kinds newer than" \
        "$PWD/$_bhq_makefile ($(bh_file_time "$_bhq_makefile")):"
    if [ -n "$1" ]; then
        bh_qmake_trace "  found $1 ($(bh_file_time "$1"))."
        return 0
    fi
    bh_qmake_trace "  none."
}

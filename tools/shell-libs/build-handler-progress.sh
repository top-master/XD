# License: Apache 2.0 without attribution need.
#
# Progress helpers of `build-handler.sh`, which sources this file after its
# own helpers: the pattern that counts the build steps, the build plan (the
# dry run of the make tool that counts them before they run), and the modes
# that `BH_PROGRESS_MODE` picks for the build-plan step and the compile step
# of `bh_make_step` (see `bh_progress_mode`).


# MARK: Rows.

# Usage: bh_progress_width
#
# Sets `_bpr_width`, once, to the column that the rows of a step wrap at:
# `BH_PROGRESS_COLUMNS` (80 by default), or one less than the width of a
# narrower terminal that tells its width.
bh_progress_width() {
    [ -z "${_bpr_width:-}" ] || return 0
    _bpr_width=${BH_PROGRESS_COLUMNS:-80}
    _bpr_size=$(stty size <&2 2>/dev/null) || _bpr_size=
    case "${_bpr_size#* }" in
        ''|*[!0-9]*|0|1)
            ;;
        *)
            if [ "${_bpr_size#* }" -le "$_bpr_width" ]; then
                _bpr_width=$((${_bpr_size#* } - 1))
            fi ;;
    esac
}

# MARK: Build steps.

# Extended-regex that matches one real build step (compile / moc / uic /
# rcc / resource / archive / link) in build-tool output, used to size and
# advance the progress bar. The `(^[[:space:]]*|/|\\)` anchor means the tool
# name must start the line, after the tab that nmake and jom print each command
# with, or follow a path separator -- so recursive driver lines
# (`cd X/ && make ...`, `... && jom ...`) are excluded, and the drivers
# themselves (make / nmake / jom) aren't in the tool list either.
#
# Covers every toolchain we drive through GitBash on Windows, plus Linux
# and macOS:
#   - Unix: clang/clang++, gcc, g++, c++, ar, ranlib, libtool, moc/uic/rcc.
#   - MinGW: adds windres (resources) + dlltool (DLL import libs).
#   - MSVC (via nmake or jom.exe): adds cl, link, lib, rc, mt, midl.
#   - `\\` in the anchor matches MSVC's backslash paths (C:\...\cl.exe);
#     MinGW/MSYS uses forward slashes, already covered by `/`.
#   - The trailing `([.]exe)?` matches the Windows `.exe` suffix.
#
# Two escaping rules keep ONE pattern usable by both `grep -E` and awk:
#   - Literal `+`/`.` are bracket classes `[+]`/`[.]`, not `\+`/`\.`:
#     awk's regex strips a lone backslash (turning `\+` into an illegal
#     `++` quantifier). `clang([+][+])?` matches `clang` and `clang++`
#     alike -- the missing `clang++` case is what made the qmake build's
#     compile step fall back to a spinner instead of a determinate bar.
#   - The literal `\\` (backslash) survives grep, but awk's `-v` collapses
#     it, so the awk consumer reads this via ENVIRON[] (no -v mangling) --
#     which is why the variable is exported.
BH_BUILD_STEP_RE='(^[[:space:]]*|/|\\)(clang([+][+])?|gcc|g[+][+]|c[+][+]|cl|link|lib|rc|mt|midl|moc|uic|rcc|windres|dlltool|libtool|ar|ranlib)([.]exe)?[[:space:]]'
export BH_BUILD_STEP_RE


# MARK: Build plan.

# Usage: bh_make_plan_count <plan-file>
#
# Prints how many build steps (see `BH_BUILD_STEP_RE`) <plan-file> holds; 0
# when it holds none, or is missing.
bh_make_plan_count() {
    # `grep -c` exits 1 on zero matches, and `|| true` keeps
    # pipefail+errexit quiet (grep still prints "0").
    _bmpc_count=$(grep -cE "$BH_BUILD_STEP_RE" "$1" 2>/dev/null || true)
    echo "${_bmpc_count:-0}"
}

# Usage: bh_make_sub_call < <line>
#
# Reads one command line that jom or nmake printed, and when it calls jom or
# nmake (a sub-make), prints the folder it runs in (empty for the current
# one) on the first line, then each argument of the call on a line of its own.
# Prints nothing for any other line. It reads the forms that qmake writes:
# `[cd <dir> && ][( <qmake call> ) && ]<jom or nmake> <arguments>`.
bh_make_sub_call() {
    awk '
        {
            line = $0
            sub(/^[[:space:]]+/, "", line)
            dir = ""
            if (match(line, /^cd [^&]+ && /)) {
                dir = substr(line, 4, RLENGTH - 7)
                sub(/[[:space:]]+$/, "", dir)
                line = substr(line, RLENGTH + 1)
            }
            if (substr(line, 1, 2) == "( ") {
                end = index(line, ") && ")
                if (end == 0) next
                line = substr(line, end + 5)
            }
            # Splits off the tool, which a path with spaces names in quotes.
            if (substr(line, 1, 1) == "\"") {
                end = index(substr(line, 2), "\"")
                if (end == 0) next
                tool = substr(line, 2, end - 1)
                line = substr(line, end + 2)
            } else {
                tool = line
                sub(/[[:space:]].*$/, "", tool)
                line = substr(line, length(tool) + 1)
            }
            tool = tolower(tool)
            sub(/^.*[\\\/]/, "", tool)
            sub(/[.]exe$/, "", tool)
            if (tool != "jom" && tool != "nmake") next
            print dir
            count = split(line, words, /[[:space:]]+/)
            for (i = 1; i <= count; i++) {
                if (words[i] != "") print words[i]
            }
        }'
}


# MARK: Progress modes.

# MARK: Build actions.

# Usage: bh_progress_actions_file <kind>
#
# Prints the file under `LOGS` that holds the <kind> (`start`, `end`,
# `estimated`, or `performed`) of this run's build actions; this script's own
# process id keeps it apart from another run's, as subshells share it.
bh_progress_actions_file() {
    echo "${LOGS:-.}/actions-$$.$1"
}

# Usage: bh_progress_actions_begin
#
# Notes when the first make run of this run started (see
# `bh_progress_plan_start`), and that a make run is going.
bh_progress_actions_begin() {
    _bpab_start=$(bh_progress_actions_file start)
    if [ ! -f "$_bpab_start" ]; then
        date +%s > "$_bpab_start"
    fi
    # The clock runs again, while this make run lasts (see
    # `bh_progress_clock`).
    rm -f "$(bh_progress_actions_file end)"
}

# Usage: bh_progress_actions_estimated <count>
#
# Adds <count> to the build actions that this run's make runs started with
# (see `bh_progress_plan_start`).
bh_progress_actions_estimated() {
    echo "$1" >> "$(bh_progress_actions_file estimated)"
}

# Usage: bh_progress_actions_performed <log>
#
# Adds the build actions that <log>, the log of one make run, shows (see
# `BH_BUILD_STEP_RE`) to this run's count, and notes when that make run ended.
bh_progress_actions_performed() {
    bh_make_plan_count "$1" >> "$(bh_progress_actions_file performed)"
    date +%s > "$(bh_progress_actions_file end)"
}

# Usage: bh_progress_actions_sum <file>
#
# Prints the sum of the counts that <file> holds, one per line.
bh_progress_actions_sum() {
    awk '{ sum += $1 } END { print sum + 0 }' "$1"
}


# MARK: On load.


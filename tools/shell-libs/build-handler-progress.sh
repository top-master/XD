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

# Usage: bh_progress_bar_cells <tail>
#
# Sets `BH_PROGRESS_BAR_CELLS` to the cells that a bar row fits (40 at most,
# 10 at least) within `_bpr_width` (see `bh_progress_width`), next to its
# 2-space indent, its brackets, its percent, and <tail>; hence a narrow
# terminal wraps the bar row in no way, which would move each later row off
# the lines it counts.
bh_progress_bar_cells() {
    bh_progress_width
    BH_PROGRESS_BAR_CELLS=$((_bpr_width - 2 - 2 - 9 - ${#1}))
    if [ "$BH_PROGRESS_BAR_CELLS" -gt 40 ]; then
        BH_PROGRESS_BAR_CELLS=40
    elif [ "$BH_PROGRESS_BAR_CELLS" -lt 10 ]; then
        BH_PROGRESS_BAR_CELLS=10
    fi
}

# Usage: bh_progress_row <step> <description>
#
# Sets `BH_PROGRESS_ROW` to what a step's row shows after its `[/]`: the
# ` (<step>/BH_STEPS_TOTAL)` counter, when both are set, then `: ` and the
# <description>, when it is set, wrapped between words at
# `BH_PROGRESS_COLUMNS` (80 by default, or one less than the width of a
# narrower terminal that tells its width), with 4 spaces in front of each further line; and
# `BH_PROGRESS_ROW_LINES` to its count of lines. Keeps the last result while
# the arguments and the total stay the same.
bh_progress_row() {
    _bpr_key="$1|${BH_STEPS_TOTAL:-}|$2"
    [ "$_bpr_key" != "${_bpr_last_key:-}" ] || return 0
    _bpr_last_key=$_bpr_key
    bh_progress_width
    _bpr_head=
    if [ -n "${BH_STEPS_TOTAL:-}" ] && [ -n "$1" ]; then
        _bpr_head=" ($1/$BH_STEPS_TOTAL)"
    fi
    # Each further line starts below the description, after the `[/]` in
    # front (3 columns) and the counter.
    _bpr_pad=$((3 + ${#_bpr_head} + 2))
    if [ -n "$2" ]; then
        _bpr_head="$_bpr_head: $2"
    fi
    # The count of lines comes first.
    _bpr_wrapped=$(printf '%s\n' "$_bpr_head" | awk -v width="$_bpr_width" \
        -v pad="$_bpr_pad" '
        NR == 1 {
            indent = sprintf("%" pad "s", "")
            row = ""
            used = 3
            if (substr($0, 1, 1) == " ") {
                row = " "
                used = 4
            }
            lines = 1
        }
        # A line break in the description starts a further line.
        NR > 1 {
            row = row "\n" indent
            used = pad
            lines++
        }
        {
            count = split($0, words, / +/)
            first = 1
            for (i = 1; i <= count; i++) {
                if (words[i] == "") continue
                if (first) {
                    row = row words[i]
                    used += length(words[i])
                    first = 0
                } else if (used + 1 + length(words[i]) <= width) {
                    row = row " " words[i]
                    used += 1 + length(words[i])
                } else {
                    row = row "\n" indent words[i]
                    used = pad + length(words[i])
                    lines++
                }
            }
        }
        END {
            print lines
            printf "%s", row
        }')
    BH_PROGRESS_ROW_LINES=${_bpr_wrapped%%[!0-9]*}
    BH_PROGRESS_ROW=${_bpr_wrapped#*
}
}

# Usage: bh_progress_row_clear
#
# Clears the row that a spinner step drew, its `BH_PROGRESS_DRAWN` lines, the
# cursor on the last of them or on the empty line below (see
# `bh_progress_unpark`), and leaves the cursor at the start of its first.
bh_progress_row_clear() {
    bh_progress_active || return 0
    bh_progress_unpark
    if [ "${BH_PROGRESS_DRAWN:-0}" -gt 1 ]; then
        bh_progress_out '\033[%dA\r\033[J' $((BH_PROGRESS_DRAWN - 1))
    else
        bh_progress_out '\r\033[K'
    fi
    BH_PROGRESS_DRAWN=0
}


# Usage: bh_progress_park
#
# Ends what `bh_progress_render` drew with an empty line, which the cursor
# then stays on, hence the rows stay off the screen's bottom edge, and the
# next render, which starts from there (see `bh_progress_unpark`), draws that
# line again too; then writes the whole frame at once (see
# `bh_progress_flush`).
bh_progress_park() {
    bh_progress_out '\n\033[K'
    bh_progress_flush
    BH_PROGRESS_PARKED=1
    bh_progress_echo_off
}

# Usage: bh_progress_out <format> [argument...]
#
# Adds the `printf` output of <format> and its arguments to the frame that
# `bh_progress_flush` writes; bash's `printf -v` starts no process for it.
bh_progress_out() {
    if [ -n "${BASH_VERSION:-}" ]; then
        # shellcheck disable=SC2059  # <format> is the caller's format.
        printf -v _bpo_part "$@"
    else
        # shellcheck disable=SC2059  # <format> is the caller's format.
        _bpo_part=$(printf "$@"; printf .)
        _bpo_part=${_bpo_part%.}
    fi
    BH_PROGRESS_OUT=${BH_PROGRESS_OUT:-}$_bpo_part
}

# Usage: bh_progress_flush
#
# Writes the frame that `bh_progress_out` built up to stderr, in one write;
# hence a Ctrl+C that stops the drawing leaves no half frame, and the cursor
# where the shell that settles the rows counts it.
bh_progress_flush() {
    [ -n "${BH_PROGRESS_OUT:-}" ] || return 0
    printf '%s' "$BH_PROGRESS_OUT" 1>&2
    BH_PROGRESS_OUT=
}

# Usage: bh_progress_unpark
#
# Moves the cursor from the empty line that `bh_progress_park` left up to
# the last row drawn, for a render to draw from there.
bh_progress_unpark() {
    if [ "${BH_PROGRESS_PARKED:-0}" -eq 1 ]; then
        bh_progress_out '\033[A'
        BH_PROGRESS_PARKED=0
    fi
}

# Usage: bh_progress_settle
#
# Leaves the settled rows of a step behind: the next output starts on the
# empty line below them (see `bh_progress_park`), or on a new line.
bh_progress_settle() {
    bh_progress_echo_on
    if [ "${BH_PROGRESS_PARKED:-0}" -eq 1 ]; then
        BH_PROGRESS_PARKED=0
        return 0
    fi
    printf '\n' 1>&2
}

# Usage: bh_progress_echo_off
#
# Stops the terminal from showing the keys typed while a step draws its rows
# (see `bh_progress_park`), hence an Enter moves the cursor off those rows in
# no way; `bh_progress_echo_on` shows them again.
bh_progress_echo_off() {
    [ -z "${_bpe_saved:-}" ] || return 0
    _bpe_saved=$(stty -g <&2 2>/dev/null) || _bpe_saved=
    [ -n "$_bpe_saved" ] || return 0
    stty -echo <&2 2>/dev/null || true
}

# Usage: bh_progress_echo_on
#
# Shows the keys typed again, which `bh_progress_echo_off` hid; as an
# `on-exit` hook (see `bh_hook`) too, hence a stop amid a step does so as well.
bh_progress_echo_on() {
    [ -n "${_bpe_saved:-}" ] || return 0
    stty "$_bpe_saved" <&2 2>/dev/null || true
    _bpe_saved=
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

# Usage: bh_make_plan [make options and targets...]
#
# Prints the commands that a dry run of `bh_make_run` (`-n`) shows for the
# given options and targets, in the current folder. GNU make runs each
# sub-make of a dry run as it is; jom and nmake run no command at all, hence
# only print the line of a sub-make. For those two, each such line (see
# `bh_make_sub_call`) is followed into its folder and Makefile too, up to
# `BH_MAKE_PLAN_DEPTH` (32 by default) sub-makes deep. The walk stops once the
# file that `BH_MAKE_PLAN_STOP` names exists, or the process that
# `BH_MAKE_PLAN_OWNER` names ended, such as a build that Ctrl+C stopped; a
# background job ignores Ctrl+C itself.
bh_make_plan() {
    bh_make_find
    if [ "$BH_MAKE_TYPE" = gnu ]; then
        bh_make_run -n "$@"
        return
    fi
    bh_make_run -n "$@" | while IFS= read -r _bmp_line; do
        if [ -n "${BH_MAKE_PLAN_STOP:-}" ] && [ -f "$BH_MAKE_PLAN_STOP" ]; then
            break
        fi
        if [ -n "${BH_MAKE_PLAN_OWNER:-}" ] \
            && ! kill -0 "$BH_MAKE_PLAN_OWNER" 2>/dev/null
        then
            break
        fi
        printf '%s\n' "$_bmp_line"
        # Only a line that names jom or nmake may call a sub-make; hence the
        # others, nearly each line, start no `awk` of `bh_make_sub_call`.
        case "$_bmp_line" in
            *[jJ][oO][mM]*|*[nN][mM][aA][kK][eE]*)
                ;;
            *)
                continue ;;
        esac
        _bmp_call=$(printf '%s\n' "$_bmp_line" | bh_make_sub_call)
        if [ -z "$_bmp_call" ] \
            || [ "${_bmp_depth:-0}" -ge "${BH_MAKE_PLAN_DEPTH:-32}" ]
        then
            continue
        fi
        # Reads the folder (first line), then the arguments (one per line).
        (
            _bmp_dir=
            _bmp_first=1
            set --
            while IFS= read -r _bmp_word; do
                if [ "$_bmp_first" -eq 1 ]; then
                    _bmp_dir=$_bmp_word
                    _bmp_first=0
                    continue
                fi
                set -- "$@" "$_bmp_word"
            done <<EOF
$_bmp_call
EOF
            if [ -n "$_bmp_dir" ]; then
                cd "$(printf '%s' "$_bmp_dir" | tr '\\' /)" || exit 0
            fi
            _bmp_depth=$((${_bmp_depth:-0} + 1))
            bh_make_plan "$@"
        )
    done
}

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

# Usage: bh_progress_mode_resolve
#
# Sets `_bhpp_mode` to the mode of the build-plan and compile steps, from
# `BH_PROGRESS_MODE` (`precise` when unset):
#   spinner   skips the build plan; the compile step spins.
#   estimate  counts the build plan hidden, beside the build; the compile step
#             shows a bar of `BH_ESTIMATED_ACTIONS` build actions at once (or
#             spins when that is unset), then of the plan's count, once the
#             plan ends.
#   precise   counts the whole build plan before the build starts, then the
#             compile step shows a bar of its count.
# Another value warns once, and counts as `precise`.
bh_progress_mode_resolve() {
    _bhpp_mode=${BH_PROGRESS_MODE:-precise}
    case "$_bhpp_mode" in
        spinner|estimate|precise)
            return 0 ;;
    esac
    if [ "${_bhpp_mode_warned:-0}" -eq 0 ]; then
        _bhpp_mode_warned=1
        bh_warning "BH_PROGRESS_MODE '$_bhpp_mode' is none of: spinner, estimate," \
            "or precise; hence it counts as precise."
    fi
    _bhpp_mode=precise
}

# Usage: bh_progress_plan_start <name> <plan-file> [make options and targets...]
#
# Runs the build-plan step of the make run <name> (see `bh_make_step`) in the
# mode of `bh_progress_mode_resolve`: writes the dry run of the make tool, for
# the given options and targets, to <plan-file>. The `spinner` mode skips the
# dry run, and `precise` waits for its end, each with a row of its own; the
# `estimate` mode starts it in the background with no row, hence takes no
# step (see `bh_progress_plan_steps` and `bh_progress_plan_tick`). Sets `BH_PLAN_TYPE` to the progress
# type that the compile step starts with (`spinner` or `bar`), and
# `BH_PLAN_MAX` to the build actions that step starts with.
bh_progress_plan_start() {
    _bhpp_name=$1
    _bhpp_file=$2
    shift 2
    bh_progress_mode_resolve
    bh_progress_actions_begin
    _bhpp_pid=
    rm -f "$_bhpp_file" "$_bhpp_file.done" "$_bhpp_file.stop" "$_bhpp_file.bar"
    BH_PLAN_TYPE=spinner
    BH_PLAN_MAX=0
    if [ "$_bhpp_mode" = estimate ]; then
        # The dry run stops early once `<plan-file>.stop` exists, or the
        # build's shell ended (see `bh_make_plan`), and touches
        # `<plan-file>.done` once it ends.
        {
            BH_MAKE_PLAN_STOP=$_bhpp_file.stop
            BH_MAKE_PLAN_OWNER=$$
            bh_make_plan "$@" 2>/dev/null > "$_bhpp_file" || true
            : > "$_bhpp_file.done"
        } &
        _bhpp_pid=$!
        if [ "${BH_ESTIMATED_ACTIONS:-0}" -gt 0 ]; then
            BH_PLAN_TYPE=bar
            BH_PLAN_MAX=$BH_ESTIMATED_ACTIONS
            bh_progress_actions_estimated "$BH_PLAN_MAX"
        fi
        return 0
    fi
    _step=$((_step + 1))
    bh_progress_setType spinner
    bh_progress_setMax 0
    if [ "$_bhpp_mode" = spinner ]; then
        bh_progress_setDescription "Makefile runner ($_bhpp_name) skipped the build plan, as BH_PROGRESS_MODE is spinner."
        bh_progress_render
        bh_progress_hide
        return 0
    fi
    bh_progress_setDescription "Makefile runner ($_bhpp_name) is computing build plan."
    # Draws the first frame, hence the row shows at once.
    bh_progress_render
    # `make -n` still remakes an out-of-date Makefile (running qmake),
    # hence it runs to its end even on Ctrl+C (see `bh_held_start`). A
    # spinner turns meanwhile when the row shows.
    bh_held_start bh_make_plan "$@" 2>/dev/null > "$_bhpp_file"
    if bh_progress_active; then
        bh_spin_until "$_bh_held_pid"
    fi
    bh_held_finish || true
    BH_PLAN_MAX=$(bh_make_plan_count "$_bhpp_file")
    rm -f "$_bhpp_file"
    # Settles the row on `[√] (N/M)`.
    bh_progress_hide
    # A plan of no build steps leaves no progress to show, hence the
    # compile step spins, rather than show a bar that never moves.
    if [ "$BH_PLAN_MAX" -gt 0 ]; then
        BH_PLAN_TYPE=bar
    fi
    bh_progress_actions_estimated "$BH_PLAN_MAX"
}

# Usage: bh_progress_plan_steps <make-runs>
#
# Takes the build-plan steps of <make-runs> make runs off `BH_STEPS_TOTAL`,
# which the templates count, when the progress mode hides them (`estimate`,
# see `bh_progress_plan_start`).
bh_progress_plan_steps() {
    bh_progress_mode_resolve
    if [ "$_bhpp_mode" = estimate ] && [ -n "${BH_STEPS_TOTAL:-}" ]; then
        BH_STEPS_TOTAL=$((BH_STEPS_TOTAL - $1))
    fi
}

# Usage: bh_progress_plan_tick
#
# Called once per spinner frame of the compile step, while a hidden build plan
# runs beside the build (see `bh_progress_plan_start`): once the plan ended,
# sets `BH_PROGRESS_MAX` to its count, turning a compile step that spins into
# a bar, and writes that count to `<plan-file>.bar` for
# `bh_progress_plan_finish`.
bh_progress_plan_tick() {
    [ -n "${_bhpp_pid:-}" ] && [ -f "$_bhpp_file.done" ] || return 0
    [ ! -f "$_bhpp_file.bar" ] || return 0
    _bhpt_count=$(bh_make_plan_count "$_bhpp_file")
    if [ "$_bhpt_count" -le 0 ]; then
        _bhpt_count=1
    fi
    if [ "${BH_PROGRESS_TYPE:-spinner}" != bar ]; then
        bh_progress_setType bar
    fi
    BH_PROGRESS_MAX=$_bhpt_count
    echo "$_bhpt_count" > "$_bhpp_file.bar"
}

# Usage: bh_progress_plan_finish
#
# Ends a hidden build plan once the build ended (see `bh_progress_plan_start`):
# stops it when it still counts, and hands the bar that a tick brought in on
# to the caller's shell, which the ticks ran beside. Removes the plan files.
bh_progress_plan_finish() {
    [ -n "${_bhpp_pid:-}" ] || return 0
    if [ ! -f "$_bhpp_file.done" ]; then
        : > "$_bhpp_file.stop"
    fi
    wait "$_bhpp_pid" 2>/dev/null || true
    _bhpp_pid=
    if [ -f "$_bhpp_file.bar" ]; then
        if [ "${BH_PROGRESS_TYPE:-spinner}" != bar ]; then
            BH_PROGRESS_TYPE=bar
            BH_PROGRESS_HEADER_PRINTED=1
            bh_progress_row "${_step:-}" "${BH_PROGRESS_DESC:-}"
            BH_PROGRESS_HEADER_LINES=$BH_PROGRESS_ROW_LINES
        fi
        BH_PROGRESS_MAX=$(cat "$_bhpp_file.bar")
    fi
    rm -f "$_bhpp_file" "$_bhpp_file.done" "$_bhpp_file.stop" "$_bhpp_file.bar"
}

# Usage: bh_progress_plan_on_exit
#
# Stops a build plan that still counts when the script exits, as an
# `on-exit` hook (see `bh_hook`); a background job ignores Ctrl+C itself.
bh_progress_plan_on_exit() {
    [ -n "${_bhpp_pid:-}" ] || return 0
    : > "$_bhpp_file.stop"
    kill "$_bhpp_pid" 2>/dev/null
}


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

# Usage: bh_progress_actions_note
#
# Prints, once a run that built anything ended well, as an `on-exit` hook
# (see `bh_hook`), how many build actions it performed and how long its make
# runs took, then, unless `--filter` or `--rebuild` narrowed the build, how
# many build actions they started with, when a mode counted or estimated
# them; `BH_LOG_NOTES=0` leaves the note out. Removes the files that hold
# them.
bh_progress_actions_note() {
    _bpan_performed=$(bh_progress_actions_file performed)
    _bpan_estimated=$(bh_progress_actions_file estimated)
    _bpan_start=$(bh_progress_actions_file start)
    _bpan_end=$(bh_progress_actions_file end)
    if [ "${BH_EXIT_STATUS:-1}" -eq 0 ] && [ -f "$_bpan_performed" ] \
        && [ "${BH_LOG_NOTES:-1}" != 0 ]
    then
        _bpan_count=$(bh_progress_actions_sum "$_bpan_performed")
        BH_PROGRESS_CLOCK=
        bh_progress_clock
        _bpan_time=$BH_PROGRESS_CLOCK
        if [ -n "${BH_FILTER:-}" ] || [ -n "${BH_REBUILD:-}" ] \
            || [ ! -f "$_bpan_estimated" ]
        then
            printf 'Note: performed %s build actions in %s duration.\n' \
                "$_bpan_count" "$_bpan_time" 1>&2
        else
            printf 'Note: performed %s build actions in %s duration\n' \
                "$_bpan_count" "$_bpan_time" 1>&2
            printf '   (while estimated %s).\n' \
                "$(bh_progress_actions_sum "$_bpan_estimated")" 1>&2
        fi
    fi
    rm -f "$_bpan_performed" "$_bpan_estimated" "$_bpan_start" "$_bpan_end"
}

# Usage: bh_progress_clock
#
# Sets `BH_PROGRESS_CLOCK` to the time from the start of this run's first make
# run (see `bh_progress_actions_begin`) to the end of its last one, or to now
# while a make run goes on, as `HH:MM:SS`; the bar row of `bh_progress_render`
# shows it, and `bh_progress_actions_note` too. Called once per spinner frame,
# it reads the clock about once a second only (each sixth frame), as each read
# runs `date`; an empty `BH_PROGRESS_CLOCK` reads it at once.
bh_progress_clock() {
    if [ -n "${BH_PROGRESS_CLOCK:-}" ] && [ $((${BH_PROGRESS_TICK:-0} % 6)) -ne 0 ]; then
        return 0
    fi
    _bpc_start=$(cat "$(bh_progress_actions_file start)" 2>/dev/null) || return 0
    _bpc_end=$(cat "$(bh_progress_actions_file end)" 2>/dev/null) || _bpc_end=$(date +%s)
    _bpc_seconds=$((_bpc_end - _bpc_start))
    BH_PROGRESS_CLOCK=$(printf '%02d:%02d:%02d' $((_bpc_seconds / 3600)) \
        $((_bpc_seconds / 60 % 60)) $((_bpc_seconds % 60)))
}

# Usage: bh_progress_actions_sum <file>
#
# Prints the sum of the counts that <file> holds, one per line.
bh_progress_actions_sum() {
    awk '{ sum += $1 } END { print sum + 0 }' "$1"
}


# MARK: On load.

bh_hook_add on-exit bh_progress_plan_on_exit
bh_hook_add on-exit bh_progress_actions_note
bh_hook_add on-exit bh_progress_echo_on

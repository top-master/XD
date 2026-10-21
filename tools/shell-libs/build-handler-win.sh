# License: Apache 2.0 without attribution need.
#
# Windows-host helpers of `build-handler.sh`, which sources this file on a
# Windows host (git-bash, MSYS, or Cygwin), after its own helpers. On load, it
# puts the POSIX tools first on `PATH` (see `bh_posix_path`), and adds an
# `on-exit` hook that checks the external tools after a failed run (see
# `bh_tool_check_on_failure`).


# The external tools the script runs; `bh_tool_check` checks each.
BH_EXTERNAL_TOOLS="awk basename cat cp curl cut date dirname find grep head ln ls
mkdir mv readlink rm sed sleep sort stat tail tar tee touch tr uname uniq"

# Usage: bh_windows_host
#
# Tells whether the shell runs on Windows (git-bash / MSYS / Cygwin).
bh_windows_host() {
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*) return 0 ;;
    esac
    return 1
}

# Usage: bh_posix_path
#
# On a Windows host (git-bash / MSYS / Cygwin), moves `/usr/bin` before the
# first Windows folder on `PATH`, since System32 carries its own `find`,
# `sort`, `tar` and `curl`, which would run in place of the POSIX ones (and
# Windows' `find` fails with nothing on stdout). Other folders, such as
# `/mingw64/bin`, keep their order. Does nothing elsewhere.
bh_posix_path() {
    bh_windows_host || return 0
    PATH=$(bh_path_before_windows /usr/bin); export PATH
}

# Usage: bh_path_before_windows <dir> [path]
#
# Prints [path] (`PATH` by default) with <dir> before its first Windows
# folder (one under a `windows` folder, in any letter case), dropping the
# later copies of <dir>; prints [path] as it is when <dir> already comes
# first. Entries match as in `bh_path_prepend`; empty entries stay.
bh_path_before_windows() {
    printf '%s:' "${2-$PATH}" | BH_PATH_DIR=$1 awk '
        function folder(entry) {
            entry = tolower(entry)
            sub(/\/+$/, "", entry)
            return entry
        }
        BEGIN {
            RS = ":"
            dir = ENVIRON["BH_PATH_DIR"]
            wanted = folder(dir)
            placed = 0
            count = 0
        }
        {
            if ($0 != "" && folder($0) == wanted) {
                if (placed) next
                placed = 1
            } else if (!placed && tolower($0) ~ /(^|\/|\\)windows(\/|\\|$)/) {
                entries[++count] = dir
                placed = 1
            }
            entries[++count] = $0
        }
        END {
            for (i = 1; i <= count; i++) {
                printf "%s%s", (i > 1 ? ":" : ""), entries[i]
            }
        }'
}

# Usage: bh_tool_check [posix-dir]
#
# Warns once about each external tool (see `BH_EXTERNAL_TOOLS`) that is
# missing, resolves to a Windows one, or, on a Windows host, resolves to
# another copy than the POSIX one in [posix-dir] (`/usr/bin` by default) that
# it shadows; such a tool fails, or worse, prints nothing, with no error of
# its own. Prints nothing when each is fine.
bh_tool_check() {
    _btc_posix=${1:-/usr/bin}
    _btc_windows=0
    if [ -n "${1:-}" ] || bh_windows_host; then
        _btc_windows=1
    fi
    _btc_lines=
    for _btc_tool in $BH_EXTERNAL_TOOLS; do
        _btc_path=$(command -v "$_btc_tool" 2>/dev/null || true)
        case "$_btc_path" in
            '')
                _btc_lines="$_btc_lines
$_btc_tool: not found." ;;
            */[Ww][Ii][Nn][Dd][Oo][Ww][Ss]/*)
                _btc_lines="$_btc_lines
$_btc_tool: runs $_btc_path, the Windows tool, not the POSIX one." ;;
            *)
                # Folders match ignoring letter case, as Windows compares them.
                _btc_lower=$(printf '%s' "$_btc_path" | tr '[:upper:]' '[:lower:]')
                _btc_want=$(printf '%s' "$_btc_posix/$_btc_tool" | tr '[:upper:]' '[:lower:]')
                if [ "$_btc_windows" -eq 1 ] && [ -x "$_btc_posix/$_btc_tool" ] \
                    && [ "${_btc_lower%.exe}" != "$_btc_want" ]; then
                    _btc_lines="$_btc_lines
$_btc_tool: runs $_btc_path, which shadows the POSIX $_btc_posix/$_btc_tool."
                fi ;;
        esac
    done
    [ -n "$_btc_lines" ] || return 0
    _btc_oldifs=$IFS
    IFS='
'
    # shellcheck disable=SC2086  # one argument per line.
    bh_warning "A tool check after the failure found issues:" ${_btc_lines#?}
    IFS=$_btc_oldifs
}

# bh_tool_check_on_failure
#
# Runs `bh_tool_check` once at the end of a failed run, as an `on-exit` hook
# (see `bh_hook`); a Ctrl+C stop (exit 130) is no failure.
bh_tool_check_on_failure() {
    if [ "$BH_EXIT_STATUS" -ne 0 ] && [ "$BH_EXIT_STATUS" -ne 130 ]; then
        bh_tool_check
    fi
}


# MARK: On load.

bh_posix_path
bh_hook_add on-exit bh_tool_check_on_failure

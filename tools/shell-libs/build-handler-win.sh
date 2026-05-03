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

# MARK: On load.


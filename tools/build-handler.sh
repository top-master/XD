# License: Apache 2.0 without attribution need.
#
# Shared build-orchestration helpers, sourced (not executed) by:
#   - `XD/build.sh`
#   - consumer build.sh files (e.g. `<App>/build.sh`)
#
# The default flow for a single-app consumer is:
#
#   #!/bin/sh
#   cd "${0%[/\\]*}" > /dev/null 2>&1
#   ROOT=$(pwd)
#
#   # Consumer-specific: detect the XD framework checkout.
#   if [ -n "${XD_ROOT:-}" ]; then XD_DIR=$XD_ROOT
#   else XD_DIR=$ROOT/../XD; fi
#   ...resolve absolute / error if missing...
#
#   BH_ROOT=$ROOT
#   BH_SCRIPT_NAME=build.sh
#   . "$XD_DIR/tools/build-handler.sh"
#
#   bh_template_app "$ROOT/<App>.pro" "$@"
#
# Frameworks that don't fit the template (XD itself) call the smaller
# helpers (bh_run, bh_make, bh_run_tests) directly. See `XD/build.sh`
# for that pattern.
#
# Caller-set env vars consumed here:
#   $BH_ROOT          absolute path of the caller's source root
#   $BH_SCRIPT_NAME   basename of the caller's script (for error reports)
#   $BH_LOG_LEVEL     log verbosity: 0 errors only (-q), 1 normal
#                     (default), 2 verbose (-v: live make + test output),
#                     3 very-verbose (-vv: also qmake's "Reading ..."
#                     firehose). bh_parse_args sets it from -q/-v/-vv; a
#                     legacy truthy $BH_VERBOSE maps to verbose.


# ---- error reporting ---------------------------------------------------

# Usage: bh_error "message" [line]
#
# Formatted to mimic compiler diagnostics so editors that read shell
# output (Qt Creator, VS Code) can jump to the caller's script line.
bh_error() {
    echo "${BH_ROOT:-.}/${BH_SCRIPT_NAME:-build.sh}:${2:-1}:1: error: $1" 1>&2
    exit 1
}

# Usage: bh_warning "first line" ["follow-up line"] ["another line"] ...
#
# Non-fatal stderr warning. Renders as:
#
#     <blank>
#     WARNING: <first line>
#              <follow-up line>
#              <another line>
#     <blank>
#
# `WARNING:` is bolded via `\033[1m`/`\033[22m` -- only when stderr
# is a tty though, so piped/captured stderr stays clean for CI
# logs. Each follow-up arg is indented 9 spaces so it aligns under
# the message column (under the space following `WARNING:`).
# Caller passes one arg per visual line; the function handles all
# padding (leading blank, trailing blank, continuation indent) so
# call sites don't keep re-implementing the same layout.
bh_warning() {
    [ "$#" -gt 0 ] || return 0
    if [ -t 2 ]; then
        _bhw_b=$(printf '\033[1m')
        _bhw_r=$(printf '\033[22m')
    else
        _bhw_b=
        _bhw_r=
    fi
    printf '\n%sWARNING:%s %s\n' "$_bhw_b" "$_bhw_r" "$1" 1>&2
    shift
    for _bhw_line in "$@"; do
        printf '         %s\n' "$_bhw_line" 1>&2
    done
    printf '\n' 1>&2
}


# ---- strict mode -------------------------------------------------------

# `pipefail` isn't in POSIX 2008 (it landed in Issue 8 / 2024), so try
# to enable it but tolerate shells that don't know it.
bh_strict_mode() {
    set -e
    (set -o pipefail) 2>/dev/null && set -o pipefail || true
    # Ctrl+C: the wrapped child in the current step has already taken the
    # signal (same process group). Settle its row on `[C]` so the user
    # sees an explicit "manual abort" glyph, then re-raise so the script
    # exits 130 instead of marching on to the next step.
    trap '_bh_on_sigint' INT
}

_bh_on_sigint() {
    bh_progress_hide C
    trap - INT
    kill -INT $$
    exit 130
}


# ---- defaults ----------------------------------------------------------

# Extended-regex that matches one real build step (compile / moc / uic /
# rcc / resource / archive / link) in build-tool output, used to size and
# advance the progress bar. The `(^|/|\\)` anchor means the tool name must
# start the line or follow a path separator -- so recursive driver lines
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
BH_BUILD_STEP_RE='(^|/|\\)(clang([+][+])?|gcc|g[+][+]|c[+][+]|cl|link|lib|rc|mt|midl|moc|uic|rcc|windres|dlltool|libtool|ar|ranlib)([.]exe)?[[:space:]]'
export BH_BUILD_STEP_RE

# Extended-regex (grep -E) matching the lines worth showing when a build
# step fails -- the "Filter" view in bh_dump_fail. Covers gcc/clang
# (`error:`, `fatal error`), MSVC (`error C1234`, `fatal error C1083`,
# linker `LNK2019`), and make/nmake/jom (`*** [target] Error 1`).
BH_BUILD_ERROR_RE='([Ee]rror:|fatal error|: error|[Ee]rror [A-Z]?[0-9]|LNK[0-9]|undefined (reference|symbol)|ld:|\*\*\* )'

# Seconds between spinner/ticker frames. One knob for every animated
# poll loop so the rotating |/-\ cadence stays uniform.
BH_SPIN_INTERVAL=0.15

# Parallel-job count for make. Picks the host's logical CPU count using
# whichever of getconf/sysctl/nproc is present; falls back to 4 on
# shells that lack all three.
bh_default_jobs() {
    if [ -z "${JOBS:-}" ]; then
        JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null \
               || sysctl -n hw.logicalcpu 2>/dev/null \
               || nproc 2>/dev/null \
               || echo 4)
    fi
}

# Pick the mkspec by host. macx-clang on Darwin, linux-g++ otherwise.
bh_default_qmakespec() {
    case "$(uname -s)" in
        Darwin) : "${QMAKESPEC:=macx-clang}";;
        *)      : "${QMAKESPEC:=linux-g++}";;
    esac
}


# ---- interactive Y/N helper -------------------------------------------

# bh_yes_no <prompt>
#
# Standard Y/N gate used by every "do you want to ..." prompt the
# scripts surface (Xcode-license accept, qmake bootstrap, XD's
# continue-or-abort prompt). Returns 0 on Y/empty/anything-not-N and
# 1 on N variants (`n`/`N`/`no`/`NO`/`No`).
#
# Under --headless (BH_HEADLESS > 0) the prompt is skipped and 0 is
# returned -- "the caller has signed up for the unattended path".
# This is what lets a single `bh_yes_no` call serve both modes.
bh_yes_no() {
    if [ "${BH_HEADLESS:-0}" -gt 0 ]; then
        return 0
    fi
    printf '%s ' "$1"
    read -r _bh_yes_no_answer
    case "$_bh_yes_no_answer" in
        n|N|no|NO|No) return 1;;
    esac
    return 0
}


# ---- macOS Xcode license -----------------------------------------------

# bh_license_message
#
# Prints a short note explaining why the macOS Xcode license must be
# accepted before the build can proceed. No-op off Darwin and no-op
# under --headless (the headless caller has already agreed to anything
# their unattended environment will surface).
#
# Pairs with `bh_yes_no` to form a license prompt -- see
# `bh_license_prompt` for the convenience wrapper.
bh_license_message() {
    [ "$(uname -s)" = "Darwin" ] || return 0
    [ "${BH_HEADLESS:-0}" -eq 0 ] || return 0
    echo
    echo "macOS requires the Xcode license to be accepted before xcodebuild"
    echo "and qmake's mac/default_pre.prf can locate Xcode -- a one-time"
    echo "system-wide step that's a no-op if you've already accepted."
}

# bh_license_accept
#
# Runs `sudo xcodebuild -license accept`. No prompt, no message --
# callers are responsible for gating (in non-headless flows the
# convention is `bh_license_message + bh_yes_no + bh_license_accept`,
# or the all-in-one `bh_license_prompt && bh_license_accept`).
#
# Off Darwin: no-op. The underlying sudo call is a no-op when the
# license is already accepted; errors are swallowed so a sudo decline
# can't break the build chain.
#
# Under `--headless` we add `-n` to sudo so the call is
# non-interactive: a missing cached credential fails immediately
# rather than blocking the build on an invisible password prompt
# (which is what happens in CI, `expect` captures, or any pipe
# where the tty isn't actually attended). If `sudo -n` fails we
# print a one-line warning telling the user to either pre-cache
# credentials (`sudo -v`) or re-run the build under sudo so the
# license-accept can complete -- then continue with the build
# (the license may already be accepted from a prior session, in
# which case the build will succeed anyway; if it isn't, the
# first compiler invocation will surface a clear xcodebuild
# license error).
bh_license_accept() {
    [ "$(uname -s)" = "Darwin" ] || return 0
    if [ "${BH_HEADLESS:-0}" -gt 0 ]; then
        if ! sudo -n xcodebuild -license accept 2>/dev/null; then
            bh_warning \
                'Could not accept the Xcode license in --headless mode.' \
                'Run the build once with sudo (or pre-cache creds with `sudo -v`)' \
                'to accept the Xcode license in --headless mode.'
        fi
        return 0
    fi
    sudo xcodebuild -license accept 2>/dev/null || true
}

# bh_license_prompt
#
# Convenience: `bh_license_message` followed by a Y/N gate. Returns 0
# when the user accepts (or under --headless, where `bh_yes_no` is
# auto-yes), 1 on N. Future callers can use this single call; today's
# callers (XD/build.sh) keep calling the two pieces explicitly for
# finer control.
bh_license_prompt() {
    bh_license_message
    bh_yes_no 'Do you want to accept Xcode license? [Y/n]'
}

# bh_license_gate
#
# Full interactive gate used by the template drivers
# (bh_template_app / bh_template_subdirs). Under --headless this is
# a no-op (bh_parse_args already pre-accepted the license). Under
# interactive runs, prints the rationale, prompts the user with the
# combined "build dir + license" question, accepts the license on Y,
# and aborts the script on N. Keeps both templates' gate semantics
# in one place so they stay in lockstep.
bh_license_gate() {
    [ "${BH_HEADLESS:-0}" -eq 0 ] || return 0
    bh_license_message
    bh_yes_no 'Continue with this build directory and accept the Xcode license? [Y/n]' \
        || { echo "Aborted."; exit 1; }
    bh_license_accept
}


# ---- macOS Xcode developer dir auto-detection --------------------------

# On Darwin, qmake's `mac/default_pre.prf` shells out to `xcode-select`
# and `xcrun` to locate Xcode. When the active selection is the bare
# Command Line Tools tree (no `xcodebuild`, no SDK metadata) but a real
# Xcode.app exists, export DEVELOPER_DIR pointing at it so those tools
# resolve correctly -- saves the user from running
# `sudo xcode-select -switch ...` system-wide.
bh_macos_developer_dir() {
    [ "$(uname -s)" = "Darwin" ] || return 0
    [ -z "${DEVELOPER_DIR:-}" ] || return 0
    case "$(xcode-select -p 2>/dev/null)" in
        */Xcode*.app/*) ;; # Already a real Xcode -- nothing to do.
        *)
            for _xc in /Applications/Xcode*.app; do
                [ -d "$_xc/Contents/Developer" ] || continue
                DEVELOPER_DIR=$_xc/Contents/Developer
                export DEVELOPER_DIR
                break
            done
            ;;
    esac
}


# ---- argument parsing --------------------------------------------------

# bh_parse_args "$@"
#
# Pulls common build-driver flags off the argument list and stores them
# in BH_* globals. Anything we don't recognise (after `--` or at the
# first non-flag) stays for the caller via BH_REMAINING_ARGS, so a
# consumer can still forward custom args to qmake.
#
# Defaults: --debug, log level 1 (normal), non-headless, no tests, no run.
#
# Flags recognised (covers every flag the consumer + XD build.sh
# files used to handle locally):
#   -q | --quiet       log level 0: errors only (no progress bar)
#   -v | --verbose     log level 2: tee live make + test output
#   -vv | --very-verbose  log level 3: also qmake's "Reading ..." firehose
#   --no-verbose       back to level 1 (normal)
#   --test | --tests   build + run the project's test suite
#   --run              launch the freshly-built app
#   --debug            build in debug mode (default)
#   --release          build in release mode
#   --headless         skip interactive prompts (XD-style scripts use this)
#   --no-progress      disable bh_make_step's live progress bar (silent
#                      make output instead, dump-on-failure as bh_run)
#   --clean            `make clean` the build dir before building
#                      (incremental wipe via the Makefile target)
#   --wipe           rm -rf the build dir before building (total
#                      wipe; no Makefile dependency)
#   --no-build         skip the build step (useful with --clean /
#                      --wipe to wipe without rebuilding)
#   --only-clean       alias for `--clean --no-build` -- the most
#                      common "just clean, don't build" shorthand
#   --build-tests      force the --test step to (re)build each test
#   --no-build-tests   force the --test step to just run a pre-built test
#                      (these default to follow --build / --no-build)
#   --qmake            force the qmake step to run even when its
#                      Makefile is up-to-date (qmake is otherwise
#                      skipped when no .pro / .pri / .prf file is
#                      newer than the Makefile under the build dir)
#   --refresh          alias for the same force-qmake behaviour --
#                      reads naturally for "give me a fresh
#                      Makefile" without implying a clean/wipe
#   --ignore-pri       narrow the freshness check to just `.pro`
#                      files (skip `.pri` and `.prf`) -- useful
#                      when an unchanged-in-effect `.pri` keeps
#                      tripping a re-qmake because some IDE keeps
#                      `touch`-ing it on save
#   --                 stop parsing; rest forwarded as BH_REMAINING_ARGS
#
# True (exit 0) when there are post-build tasks to run -- i.e. --test or
# --run was requested. Such tasks consume the executable's TARGET name
# and make the build steps a prerequisite rather than the end of the run.
bh_has_tasks() {
    [ "${BH_RUN_TESTS:-0}" -gt 0 ] || [ "${BH_RUN_TARGET:-0}" -gt 0 ]
}

# True (exit 0) when the lifecycle can stop at the given stage with
# nothing left undone. The argument is the current stage (hook keyword):
#   pre-build   after the wipe/clean phase, before qmake -- can stop when
#               --no-build was asked, no `--qmake` Makefile regen is due,
#               and there are no post-build tasks (--test/--run).
#   post-qmake  after a `--qmake --no-build` Makefile regen -- can stop
#               unless post-build tasks still need the build artifacts.
bh_can_exit() {
    case "$1" in
        pre-build)
            [ "${BH_NO_BUILD:-0}" -gt 0 ] \
                && [ "${BH_FORCE_QMAKE:-0}" -eq 0 ] \
                && ! bh_has_tasks
            ;;
        post-qmake)
            ! bh_has_tasks
            ;;
        *)
            bh_error "bh_can_exit: unknown stage '$1'"
            ;;
    esac
}

# True (exit 0) when $1 is an absolute path, recognising both unix
# (/path) and Windows forms (C:/path, C:\path, and \\host\share UNC) --
# the Windows ones matter under a shell like Git Bash. Backslashes are
# written `\\` (one literal backslash) outside the brackets to dodge
# shell glob-escaping quirks inside bracket expressions.
bh_is_path_absolute() {
    case "${1:-}" in
        /* | [A-Za-z]:/* | [A-Za-z]:\\* | \\\\*) return 0;;
        *) return 1;;
    esac
}

# True (exit 0) when $1 is a real test project -- one that declares
# `CONFIG += testcase`, the marker the test runner keys on. The first
# grep narrows to CONFIG assignments, the second checks `testcase` as a
# whole token (so a value like `testcase_exceptions` does not count).
# Used to keep dummy `.pro` fixtures a test feeds to qmake (e.g. the
# qmake test's testdata/*.pro) out of a glob's results.
bh_is_testcase() {
    grep -E '^[[:space:]]*CONFIG[[:space:]]*[-+*]?=' "$1" 2>/dev/null \
        | grep -Eq '(^|[[:space:]])testcase([[:space:]]|$)'
}

# Consume an optional test-project argument shared by --test and
# --test-review. $1 may be a `.pro` path or a glob pattern (quote a glob
# so the calling shell passes it through, e.g. `--test '**/*qmake*'`).
#
# A literal path stores that one project in BH_TEST_ROOT. A glob stores
# every `.pro` whose path matches -- newline-separated and sorted, so the
# order is stable regardless of the filesystem's find order -- and sets
# BH_TEST_GLOB=1. Real-ness is deliberately NOT checked here: deferring
# it to the test step lets that step vet the upcoming matches in the
# background while the current one builds and runs (see the run loop).
# It reports success so the caller knows to `shift` the arg. A flag, an
# empty arg, or anything that is neither a `.pro` nor a glob leaves
# BH_TEST_ROOT untouched and reports failure. Runs in the parser's own
# shell (not a subshell) so bh_error can abort.
bh_take_test_pro() {
    case "${1:-}" in
        '' | -*)               return 1 ;;  # nothing, or another flag
        *.pro) ;;                            # a .pro path
        *"*"* | *"?"* | *"["*) ;;            # a glob pattern
        *)                     return 1 ;;   # neither -> not ours
    esac
    _bhtp_arg=$1

    # A literal, existing file wins outright (covers full .pro paths and
    # a glob the calling shell already expanded to one file). Keep it if
    # absolute; otherwise make it absolute against the current dir (a bare
    # $(pwd) prefix would mangle a native path under a Windows shell --
    # hence bh_is_path_absolute).
    if [ -f "$_bhtp_arg" ]; then
        if bh_is_path_absolute "$_bhtp_arg"; then
            BH_TEST_ROOT=$_bhtp_arg
        else
            BH_TEST_ROOT=$(pwd)/$_bhtp_arg
        fi
        return 0
    fi

    # Otherwise treat it as a glob and collect every `.pro` whose path
    # matches. The pattern need not spell out the extension (`**/*qmake*`
    # finds qmake `.pro`s); it is matched against each file's path
    # relative to the root via `case`, where `*` (and `**`) span
    # directory separators. `find | sort` pins the order; `set -f` stops
    # the find output from being re-globbed (`case` still patterns).
    # Real-ness is left to the test step (see BH_TEST_GLOB above).
    _bhtp_roots=
    _bhtp_count=0
    _bhtp_oldifs=$IFS
    set -f
    IFS='
'
    for _bhtp_pro in $(find "$BH_ROOT" -name '*.pro' \
            ! -path '*/build/*' ! -path '*/.git/*' 2>/dev/null | sort); do
        _bhtp_rel=${_bhtp_pro#"$BH_ROOT"/}
        case "$_bhtp_rel" in
            $_bhtp_arg) ;;
            *) continue ;;
        esac
        _bhtp_count=$((_bhtp_count + 1))
        _bhtp_roots="${_bhtp_roots:+$_bhtp_roots
}$_bhtp_pro"
    done
    IFS=$_bhtp_oldifs
    set +f

    if [ "$_bhtp_count" -eq 0 ]; then
        bh_error "--test: nothing under '$BH_ROOT' matches the glob '$_bhtp_arg'"
    fi
    BH_TEST_ROOT=$_bhtp_roots
    BH_TEST_GLOB=1
    BH_TEST_PATTERN=$_bhtp_arg   # for the "Searching for test-pattern" spinner
}

# Template-sensitive: when `$BH_TEMPLATE` is set to "subdirs", the
# `--run` flag also consumes the next positional argument as the
# relative subdir path to launch -- subdirs trees don't have a
# single top-level TARGET, so the user has to point us at one.
# Stored in `BH_RUN_TARGET_PATH`. Under `BH_TEMPLATE=app` (or
# unset), `--run` consumes nothing extra -- behaviour unchanged.
bh_parse_args() {
    # Use `:=` defaults so a caller that pre-sets any BH_* (env var,
    # wrapper script, nested bh_template_app call) survives this
    # initialisation -- only unset values get the default. Cmdline
    # flags below still win because they assign with plain `=`.
    : "${BH_MODE:=debug}"
    # Log verbosity is a level: 0 = errors only, 1 = normal (default),
    # 2 = verbose (-v: live make + test output), 3 = very-verbose (-vv:
    # also qmake's recursive "Reading ..." firehose). Back-compat: an old
    # truthy BH_VERBOSE maps to verbose. Normalise anything unexpected.
    case "${BH_VERBOSE:-}" in '' | 0) ;; *) : "${BH_LOG_LEVEL:=2}" ;; esac
    case "${BH_LOG_LEVEL:-}" in 0 | 1 | 2 | 3) ;; *) BH_LOG_LEVEL=1 ;; esac
    : "${BH_HEADLESS:=0}"
    : "${BH_RUN_TESTS:=0}"
    : "${BH_RUN_TARGET:=0}"
    : "${BH_NO_PROGRESS:=0}"
    : "${BH_CLEAN:=0}"
    : "${BH_WIPE:=0}"
    : "${BH_NO_BUILD:=0}"
    : "${BH_FORCE_QMAKE:=0}"
    : "${BH_IGNORE_PRI:=0}"
    : "${BH_TEST_GLOB:=0}"

    # --test-review is a review shortcut that pre-sets headless + verbose
    # + no-build. We apply those defaults *before* the parse loop so that
    # the loop -- which sees every flag, including ones written before
    # --test-review -- can override any of them through the same
    # variables (--no-verbose -> BH_LOG_LEVEL=1, --headed -> BH_HEADLESS=0,
    # --build -> BH_NO_BUILD=0). No extra "force" variables needed.
    for _bharg in "$@"; do
        if [ "$_bharg" = "--test-review" ]; then
            BH_HEADLESS=1
            BH_LOG_LEVEL=2
            BH_NO_BUILD=1
            break
        fi
    done

    while [ $# -gt 0 ]; do
        case "$1" in
            # Log level: -q = 0 (errors only), default 1 (normal),
            # -v = 2 (live make + test output), -vv = 3 (also the qmake
            # "Reading ..." firehose). -v never downgrades a higher -vv.
            -q|--quiet)          BH_LOG_LEVEL=0; shift ;;
            -v|--verbose)
                case "${BH_LOG_LEVEL:-1}" in 0|1|2) BH_LOG_LEVEL=2 ;; esac
                shift ;;
            -vv|--very-verbose)  BH_LOG_LEVEL=3; shift ;;
            --test|--tests)
                # An optional `.pro` path or glob right after --test
                # overrides the default test root; `--test` alone (or
                # before another flag) keeps it.
                BH_RUN_TESTS=1; shift
                bh_take_test_pro "${1:-}" && shift
                ;;
            --test-review)
                # Review shortcut == --test <pro> --headless --verbose
                # --no-build. The three defaults were pre-set above; here
                # we just enable tests and take the optional `.pro` path or
                # glob. Flip any default with its opposite flag (any order).
                BH_RUN_TESTS=1; shift
                bh_take_test_pro "${1:-}" && shift
                ;;
            # Opposites of the --test-review defaults -- they just clear
            # the same variables the shortcut set.
            --build)                BH_NO_BUILD=0; shift;;
            --headed|--interactive) BH_HEADLESS=0; shift;;
            --no-verbose)           BH_LOG_LEVEL=1; shift;;
            --run)
                BH_RUN_TARGET=1; shift
                # Subdirs template: --run needs a subdir path. App
                # template (default): nothing to consume.
                if [ "${BH_TEMPLATE:-app}" = "subdirs" ]; then
                    [ $# -gt 0 ] || bh_error "--run needs a subdir path when BH_TEMPLATE=subdirs"
                    # Two guards against silently eating the next
                    # flag as a path:
                    #   1) reject anything starting with `-` -- a
                    #      legitimate subdir name wouldn't.
                    #   2) require the path to resolve to a real
                    #      directory under $BH_ROOT -- catches
                    #      typos and the "missing arg" case where
                    #      the user invoked `--run` last.
                    case "$1" in
                        -*) bh_error "--run expects a subdir path next, got flag '$1'";;
                    esac
                    [ -d "$BH_ROOT/$1" ] || \
                        bh_error "--run path '$1' does not exist under '$BH_ROOT'"
                    BH_RUN_TARGET_PATH=$1; shift
                fi
                ;;
            --debug)          BH_MODE=debug; shift;;
            --release)        BH_MODE=release; shift;;
            --headless)       BH_HEADLESS=1; shift;;
            --no-progress)    BH_NO_PROGRESS=1; shift;;
            # --libc=<static|musl|...>: how the C runtime is linked (resolved
            # below into BH_LIBC_CC / BH_LIBC_LDFLAGS that build scripts apply).
            # --musl is an alias for --libc=musl.
            --libc=*)         BH_LIBC="${1#*=}"; shift;;
            --musl)           BH_LIBC=musl; shift;;
            --clean)          BH_CLEAN=1; shift;;
            --wipe)         BH_WIPE=1; shift;;
            --no-build)       BH_NO_BUILD=1; shift;;
            # --only-clean expands to the `--clean --no-build` combo
            # -- the common "wipe without rebuild" shorthand.
            --only-clean)     BH_CLEAN=1; BH_NO_BUILD=1; shift;;
            # Whether the test step (re)builds each test or just runs an
            # already-built one. Left unset here, it defaults to follow the
            # main build (see the resolution after the loop); these flags
            # force it on/off independently.
            --build-tests)    BH_BUILD_TESTS=1; shift;;
            --no-build-tests) BH_BUILD_TESTS=0; shift;;
            # Two spellings for the same force-qmake behaviour.
            --qmake|--refresh) BH_FORCE_QMAKE=1; shift;;
            --ignore-pri)     BH_IGNORE_PRI=1; shift;;
            -d|--directory)
                # Override BUILD_DIR; consumed by all templates' :- default.
                # Absolutise a relative value: templates `cd` around before
                # reusing BUILD_DIR, so a relative path would later resolve
                # against the wrong directory.
                [ $# -ge 2 ] || bh_error "$1 requires a value"
                if bh_is_path_absolute "$2"; then
                    BUILD_DIR=$2
                else
                    BUILD_DIR=$(pwd)/$2
                fi
                shift 2;;
            --)               shift; break;;
            *)                break;;
        esac
    done
    BH_REMAINING_ARGS="$*"

    # --libc / --musl (parsed above): resolve the chosen C runtime into the
    # compiler a build should prefer (BH_LIBC_CC, may be empty) and the extra
    # link flags to add (BH_LIBC_LDFLAGS):
    #   static -- the compiler's default libc, linked statically.
    #   musl   -- musl libc, statically; needs a musl C++ toolchain (default
    #             x86_64-linux-musl-g++, override via CXX -- e.g. CXX=g++ on a
    #             musl-native host such as Alpine).
    # Anything else (or unset) keeps the compiler default, dynamically linked.
    BH_LIBC_CC=; BH_LIBC_LDFLAGS=
    case "${BH_LIBC:-}" in
        static) BH_LIBC_LDFLAGS="-static" ;;
        musl)   BH_LIBC_CC="x86_64-linux-musl-g++"; BH_LIBC_LDFLAGS="-static" ;;
    esac
    # Resolve whether the test step builds each test. Unless --build-tests
    # / --no-build-tests set it explicitly (or a caller pre-set it), it
    # follows the now-final main build: build tests when building, skip
    # (just run) under --no-build. From here on, test logic checks only
    # BH_BUILD_TESTS -- never BH_NO_BUILD.
    if [ -z "${BH_BUILD_TESTS:-}" ]; then
        if [ "${BH_NO_BUILD:-0}" -gt 0 ]; then
            BH_BUILD_TESTS=0
        else
            BH_BUILD_TESTS=1
        fi
    fi

    # Only --test / --run consume the executable's TARGET name, so mark it
    # `pending` for lazy resolution (bh_target_resolve runs `qmake -E` on
    # first fetch). A plain build leaves it unset and skips that step.
    # `:=` so a caller that pre-set BH_TARGET_NAME keeps its literal.
    if bh_has_tasks; then
        : "${BH_TARGET_NAME:=pending}"
    fi

    # Headless = "the caller has taken responsibility for unattended
    # behavior." Pre-accept the macOS Xcode license here so downstream
    # steps (qmake's mac/default_pre.prf, xcodebuild invocations the
    # build calls) don't trip on the un-accepted gate. Interactive runs
    # keep wrapping `bh_license_accept` in their own Y/N prompt.
    #
    # Skip it under --no-build: nothing gets compiled then, so the
    # precaution -- and its "could not accept the license" warning when
    # passwordless sudo isn't available -- is just noise.
    if [ "$BH_HEADLESS" -gt 0 ] && [ "${BH_NO_BUILD:-0}" -eq 0 ]; then
        bh_license_accept
    fi
}


# ---- per-step log cache + run helper -----------------------------------

# bh_logs_init "$DIR"
#
# Per-step logs go to disk (no risk of overflowing a shell variable for
# a long build). Each invocation is a fresh re-build, so wipe the prior
# logs up front -- only deletes existing `*.log` files; the folder
# itself is reused.
bh_logs_init() {
    LOGS=$1
    mkdir -p "$LOGS"
    rm -f "$LOGS"/*.log
    # Only init `_step` if the template hasn't already started
    # counting. `--wipe` runs *before* bh_logs_init and bumps
    # `_step` to 1 for the `[√] (1/N)` row; clobbering it here
    # would push every subsequent step's counter down by one.
    : "${_step:=0}"
}

# ---- progress bar (reusable) ------------------------------------------

# Stateful progress API. Four methods, single source of truth for the
# bar format. Both `bh_run` (qmake/test-qmake/etc -- spinner mode)
# and `bh_make_step` (determinate mode, fed by parsed make output)
# drive the same renderer so visuals stay consistent.
#
#   bh_progress_setType <t>         Pick the display shape:
#                                     "spinner" -- single line, format
#                                       `  [/] (step/total): <desc>`
#                                       (description on same row).
#                                     "bar"     -- two-row shape:
#                                       description prints once as a
#                                       header line above, the bar
#                                       updates in place on the next
#                                       row as `  [bar] X.XX % (V/M)`
#                                       where V/M is value/max (e.g.
#                                       "(745/2466)" for "compiled
#                                       745 of 2466 files").
#                                   Resets the header-printed flag so
#                                   the next render emits the header.
#   bh_progress_setMax <N>          Set the determinate denominator
#                                   (use with `setType "bar"`). Resets
#                                   value to 0 and "arms" hide so the
#                                   next bh_progress_hide actually
#                                   advances to a new line (otherwise
#                                   it's a no-op, so unrelated callers
#                                   don't eat a blank line). For spinner
#                                   mode, call `setMax 0` after
#                                   `setType "spinner"`.
#   bh_progress_setValue <V>        Update the count without rendering.
#   bh_progress_setDescription <s>  Store <s> for the renderer to use --
#                                   either inline (spinner mode) or as
#                                   the header line above the bar (bar
#                                   mode). The bar mode header is
#                                   printed once per `setType` call.
#   bh_progress_render              Draw the row on stderr (clears via
#                                   \r\033[K). No-op when stderr isn't
#                                   a TTY or BH_NO_PROGRESS=1. Bar mode
#                                   embeds the `|/-\` spinner at the
#                                   filled portion's leading edge
#                                   (`[##/-------]`); spinner mode is
#                                   just the spinner char alone.
#   bh_progress_hide                Re-render the row once with the
#                                   "done" symbol -- the bar/spinner
#                                   gets replaced by the radical sign
#                                   `[√]` -- then `\n` to advance the
#                                   cursor so the row stays on screen
#                                   as a completed-phase log line. The
#                                   next phase's row appears below.
#                                   No-op unless setMax was called
#                                   since the last hide.
#
# State is plain shell vars (BH_PROGRESS_MAX / _VALUE / _ARMED), so
# a subshell that drives the methods (e.g. inside a pipe) sees the
# correct max but its value updates don't propagate back -- that's
# fine for the renders themselves; for end-of-step `printf '\n'`
# the parent's _ARMED flag is what matters.

bh_progress_setMax() {
    BH_PROGRESS_MAX=$1
    BH_PROGRESS_VALUE=0
    BH_PROGRESS_DONE=0
    BH_PROGRESS_ARMED=1
}

bh_progress_setValue() {
    BH_PROGRESS_VALUE=$1
}

bh_progress_setDescription() {
    BH_PROGRESS_DESC=$1
}

# bh_progress_setType <type>
#
# Switches the renderer between display shapes:
#   "spinner" -- single-line `  [/] (step/total): description.`
#                (description rides on the same row as the spinner)
#   "bar"     -- two-line shape: description prints once as a header
#                line, then `  [bar] X.XX %  (step/total)` updates in
#                place on the next row. Re-printing of the header is
#                tracked via BH_PROGRESS_HEADER_PRINTED so subsequent
#                renders don't keep re-emitting the header text.
#
# Resets the header-printed flag so the next `bh_progress_render`
# under "bar" emits the description header on a fresh row.
bh_progress_setType() {
    BH_PROGRESS_TYPE=$1
    BH_PROGRESS_HEADER_PRINTED=0
}

# bh_progress_format_bracket
#
# Returns (on stdout) the content that goes between the `[` and `]`
# in the next render -- a single spinner char in spinner mode, or
# the 40-char bar-with-embedded-spinner in bar mode, or `√` in the
# done state. Pure formatting, no TTY/no-progress gating, so this
# is the testable seam the spec exercises.
bh_progress_format_bracket() {
    case $((${BH_PROGRESS_TICK:-0} % 4)) in
        0) _bhpf_spin='|';;
        1) _bhpf_spin='/';;
        2) _bhpf_spin='-';;
        3) _bhpf_spin='\';;
    esac
    if [ "${BH_PROGRESS_DONE:-0}" -eq 1 ]; then
        printf '%s' "${BH_PROGRESS_DONE_GLYPH:-√}"
        return 0
    fi
    if [ "${BH_PROGRESS_TYPE:-spinner}" = "bar" ] \
        && [ "${BH_PROGRESS_MAX:-0}" -gt 0 ]; then
        # 40-char bar. Centi-percent / 250 maps to 0..40 hashes.
        # At full fill the spinner sits at the last position so its
        # rotating frame is still visible (matters most for no-op
        # builds where the bar lands at 100% immediately).
        _bhpf_c=$((BH_PROGRESS_VALUE * 10000 / BH_PROGRESS_MAX))
        [ "$_bhpf_c" -gt 10000 ] && _bhpf_c=10000
        _bhpf_bars=$((_bhpf_c / 250))
        if [ "$_bhpf_bars" -ge 40 ]; then
            _bhpf_hash=$(printf '%*s' 39 '' | tr ' ' '#')
            printf '%s%s' "$_bhpf_hash" "$_bhpf_spin"
        else
            _bhpf_hash=$(printf '%*s' "$_bhpf_bars" '' | tr ' ' '#')
            # Bar's empty cells use `_` rather than `-` so the
            # `-` frame of the embedded |/-\ spinner stays
            # visually distinct against the bar's trough.
            _bhpf_dash=$(printf '%*s' $((40 - _bhpf_bars - 1)) '' | tr ' ' '_')
            printf '%s%s%s' "$_bhpf_hash" "$_bhpf_spin" "$_bhpf_dash"
        fi
    else
        # Spinner mode: a single rotating frame.
        printf '%s' "$_bhpf_spin"
    fi
}

# bh_progress_active
#
# True (0) when a live progress display should be drawn -- progress
# isn't disabled (`--no-progress`), the log level is at least normal
# (level 0 / -q shows errors only), and stderr is an interactive tty.
# The single predicate every render/poll site gates on; note it does
# NOT cover bh_warning's `-t 2` check, which gates ANSI bold for the
# warning text and should fire even under --no-progress.
bh_progress_active() {
    [ "${BH_NO_PROGRESS:-0}" -eq 0 ] && [ "${BH_LOG_LEVEL:-1}" -ge 1 ] \
        && [ -t 2 ]
}

bh_progress_render() {
    bh_progress_active || return 0
    # Tick advances per render call -- both modes animate |/-\
    # from the same counter via bh_progress_format_bracket.
    BH_PROGRESS_TICK=$((${BH_PROGRESS_TICK:-0} + 1))
    _bhp_bracket=$(bh_progress_format_bracket)

    if [ "${BH_PROGRESS_TYPE:-spinner}" = "bar" ]; then
        if [ "${BH_PROGRESS_DONE:-0}" -eq 1 ]; then
            # DONE state collapses the two-row bar layout into a
            # single `[√] (N/M): description` row -- matches the
            # spinner-mode rows' shape so a scrollback of completed
            # steps reads uniformly. If the description header was
            # printed above the bar row during the active phase
            # (header row at line N, bar row at line N+1), we
            # clear both and write the settled row on the header
            # row. Cursor sequence:
            #   \r          col 0 of bar row
            #   \033[K      clear bar row
            #   \033[1A     up to header row
            #   \033[K      clear header row
            #   \r          col 0
            if [ "${BH_PROGRESS_HEADER_PRINTED:-0}" -eq 1 ]; then
                printf '\r\033[K\033[1A\033[K\r' 1>&2
                BH_PROGRESS_HEADER_PRINTED=0
            else
                printf '\r\033[K' 1>&2
            fi
            printf '[%s]' "$_bhp_bracket" 1>&2
            if [ -n "${BH_STEPS_TOTAL:-}" ] && [ -n "${_step:-}" ]; then
                printf ' (%d/%d)' "$_step" "$BH_STEPS_TOTAL" 1>&2
            fi
            if [ -n "${BH_PROGRESS_DESC:-}" ]; then
                printf ': %s' "$BH_PROGRESS_DESC" 1>&2
            fi
        else
            # Active bar: description sits on its own header row,
            # the bar row updates in place on the next row. The
            # split is needed because the bar row carries a live
            # percent that updates per compile event -- packing
            # description + bar + percent + counter onto one line
            # blows past terminal width and visually thrashes.
            if [ "${BH_PROGRESS_HEADER_PRINTED:-0}" -eq 0 ] \
                && [ -n "${BH_PROGRESS_DESC:-}" ]; then
                printf '\r\033[K%s\n' "$BH_PROGRESS_DESC" 1>&2
                BH_PROGRESS_HEADER_PRINTED=1
            fi
            # Bar mode is only used when progress is *known* and
            # computable -- unknown-progress phases (no-op builds,
            # indeterminate work) go through spinner mode instead,
            # so they never hit this branch and never see a
            # misleading `100.00 %`.
            _bhp_c=$((BH_PROGRESS_VALUE * 10000 / BH_PROGRESS_MAX))
            [ "$_bhp_c" -gt 10000 ] && _bhp_c=10000
            _bhp_int=$((_bhp_c / 100))
            _bhp_frac=$((_bhp_c % 100))
            printf '\r\033[K  [%s] %3d.%02d %%' \
                "$_bhp_bracket" "$_bhp_int" "$_bhp_frac" 1>&2
            if [ -n "${BH_STEPS_TOTAL:-}" ] && [ -n "${_step:-}" ]; then
                printf ' (%d/%d)' "$_step" "$BH_STEPS_TOTAL" 1>&2
            fi
        fi
    else
        # Spinner mode: single line, `[sym] (step/total): desc`
        # (flush-left -- bar mode keeps the 2-space indent so the
        # bar row sits visually nested under its description header,
        # but spinner-mode rows are top-level and don't want that).
        printf '\r\033[K[%s]' "$_bhp_bracket" 1>&2
        if [ -n "${BH_STEPS_TOTAL:-}" ] && [ -n "${_step:-}" ]; then
            printf ' (%d/%d)' "$_step" "$BH_STEPS_TOTAL" 1>&2
        fi
        if [ -n "${BH_PROGRESS_DESC:-}" ]; then
            printf ': %s' "$BH_PROGRESS_DESC" 1>&2
        fi
    fi
}

bh_progress_hide() {
    [ "${BH_PROGRESS_ARMED:-0}" -eq 1 ] || return 0
    BH_PROGRESS_ARMED=0
    bh_progress_active || return 0
    # Render once more in "done" mode so the row's bar/spinner is
    # replaced by a status glyph, then `\n` to preserve the row and
    # advance the cursor onto a fresh line for the next phase. Glyph
    # defaults to `√` (success); callers pass `X` on step failure and
    # `C` on user Ctrl+C so a scrollback reads as √/X/C at a glance.
    BH_PROGRESS_DONE=1
    BH_PROGRESS_DONE_GLYPH=${1:-√}
    bh_progress_render
    printf '\n' 1>&2
}

# bh_spin_until <pid>
#
# Foreground spinner: poll until the background <pid> exits, advancing
# the rotating frame once per BH_SPIN_INTERVAL tick. `kill -0` is a
# liveness probe (delivers no signal). The VALUE bump is a no-op in
# spinner mode (the frame comes from BH_PROGRESS_TICK) -- it only
# matters if a caller is in bar mode, so it's harmless here.
bh_spin_until() {
    while kill -0 "$1" 2>/dev/null; do
        BH_PROGRESS_VALUE=$((BH_PROGRESS_VALUE + 1))
        bh_progress_render
        sleep "$BH_SPIN_INTERVAL"
    done
}

# bh_filter_errors <logfile>
#
# Print just the error-ish lines (BH_BUILD_ERROR_RE) of a step log to
# stderr. Falls back to the log's tail when nothing matches, so a
# failure never shows an empty filter.
bh_filter_errors() {
    if ! grep -nE "$BH_BUILD_ERROR_RE" "$1" 1>&2; then
        printf '  (no recognizable error lines; showing last 40 of %s)\n' "$1" 1>&2
        tail -n 40 "$1" 1>&2
    fi
}

# bh_dump_fail <logfile> <status>
#
# Shared tail of every "quiet step failed" path. Rather than spewing the
# whole captured log, name the log file and -- when interactive -- offer
# to (O)pen it in nano, (F)ilter it to just the error lines, or (I)gnore
# and continue to the abort. Headless / non-tty runs can't prompt, so
# they print the filtered errors (useful in CI without flooding it).
# Either way the script then aborts with the step's exit status.
bh_dump_fail() {
    _bhdf_log=$1
    _bhdf_status=$2

    printf '\nBuild step failed (exit %s). Log:\n  %s\n' \
        "$_bhdf_status" "$_bhdf_log" 1>&2

    if [ "${BH_HEADLESS:-0}" -gt 0 ] || [ ! -t 0 ] || [ ! -t 2 ]; then
        bh_filter_errors "$_bhdf_log"
        exit "$_bhdf_status"
    fi

    # Interactive: loop so Filter-then-Open works; Ignore (or empty)
    # falls through to the abort below.
    while :; do
        printf 'Open in nano, Filter errors, or Ignore? [o/f/i] ' 1>&2
        read -r _bhdf_ans </dev/tty || _bhdf_ans=i
        case "$_bhdf_ans" in
            o|O)
                if command -v nano >/dev/null 2>&1; then
                    nano "$_bhdf_log" </dev/tty >/dev/tty 2>&1
                else
                    "${EDITOR:-vi}" "$_bhdf_log" </dev/tty >/dev/tty 2>&1
                fi
                ;;
            f|F) bh_filter_errors "$_bhdf_log" ;;
            i|I|"") break ;;
            *) printf '  please answer o, f, or i.\n' 1>&2 ;;
        esac
    done
    exit "$_bhdf_status"
}


# ---- bh_run -----------------------------------------------------------

# bh_run <name> <cmd...>
#
# Run a named step. Stream goes to `$LOGS/<NN>-<name>.log`. Three
# display modes:
#
#   - Verbose mode (BH_LOG_LEVEL >= 2): tee output to the terminal so
#     the user sees everything; no spinner (would clash with the
#     stream).
#   - Quiet + interactive (TTY stderr, BH_NO_PROGRESS=0): run the
#     command in the background and spin the indeterminate progress
#     bar on the foreground while we poll. Wakes once per second so
#     a long step (qmake on a big subdirs tree) shows it's alive.
#   - Quiet + non-interactive (CI, redirected log, --no-progress):
#     plain silent run; dump the log to stderr only on failure.
#
# All three paths exit with the wrapped command's status, dumping
# the captured log on failure so a non-verbose step never hides
# its error message.

# bh_run_silent_fg <description> <cmd...>
#
# Wrap a *foreground* command with a counted spinner row -- the
# display reads `[/] (N/M): <description>` and `_step` advances
# just like for any other phase. "Silent" here refers to the
# absence of a log file -- the wrapped command's stdout/stderr
# stays attached to the caller's, not captured to disk.
#
# Why foreground (not the bh_run background+poll pattern)?
# Because the wrapped commands set caller-visible env vars
# (`bh_parse_target` -> `BH_TARGET_NAME`; `bh_searchpath_plugins`
# -> `QT_PLUGINS_STAGE`). Backgrounding them would lose those
# assignments to a subshell. Instead the spinner runs in a small
# background ticker subshell, killed when the command returns.
#
# set -e is toggled off around the wrapped command so a non-zero
# return surfaces as our own `return $status` instead of an
# opaque function-aborted-mid-spinner state -- the caller can
# decide whether to exit or recover.
bh_run_silent_fg() {
    _bhsf_desc=$1
    shift
    if ! bh_progress_active; then
        "$@"
        return $?
    fi
    _step=$((${_step:-0} + 1))
    bh_progress_setType spinner
    bh_progress_setMax 0
    bh_progress_setDescription "$_bhsf_desc"
    bh_progress_render
    ( while sleep "$BH_SPIN_INTERVAL"; do
          BH_PROGRESS_VALUE=$((BH_PROGRESS_VALUE + 1))
          bh_progress_render
      done ) &
    _bhsf_pid=$!
    set +e
    "$@"
    _bhsf_status=$?
    set -e
    kill "$_bhsf_pid" 2>/dev/null || true
    wait "$_bhsf_pid" 2>/dev/null || true
    # Ctrl+C is handled by the global INT trap (renders `[C]` and exits);
    # here we only distinguish clean success (√) from runner-reported
    # failure (X) so a scrollback of completed steps reads at a glance.
    if [ "$_bhsf_status" -eq 0 ]; then
        bh_progress_hide
    else
        bh_progress_hide X
    fi
    return "$_bhsf_status"
}

bh_run() {
    _name=$1; shift
    _step=$((_step + 1))
    _log=$LOGS/$(printf '%02d-%s.log' "$_step" "$_name")

    # Verbose (level 2+) tees live output; normal/quiet logs silently.
    if [ "${BH_LOG_LEVEL:-1}" -ge 2 ]; then
        # `pipefail` (set best-effort in bh_strict_mode) makes the
        # pipeline's status the wrapped command's, not tee's -- so
        # propagate it. A bare `return 0` here would hide a failed step
        # from any caller that checks (e.g. the per-test sweep) whenever
        # -e is off, such as inside a subshell used as a condition.
        "$@" 2>&1 | tee "$_log"
        return $?
    fi

    if bh_progress_active; then
        bh_progress_setType spinner
        bh_progress_setMax 0
        bh_progress_setDescription "$_name is running."
        "$@" >"$_log" 2>&1 &
        _pid=$!
        # Initial frame so something appears immediately; bh_spin_until
        # then animates while the background command runs.
        # See also docs of: bh_spin_until, for the kill -0 liveness poll.
        bh_progress_render
        bh_spin_until "$_pid"
        _status=0
        wait "$_pid" || _status=$?
        if [ "$_status" -eq 0 ]; then
            bh_progress_hide
        else
            bh_progress_hide X
            bh_dump_fail "$_log" "$_status"
        fi
    else
        if "$@" >"$_log" 2>&1; then
            :
        else
            _status=$?
            bh_dump_fail "$_log" "$_status"
        fi
    fi
}

# bh_run_qmake <name> <pro> [extra-qmake-args...]
#
# Quietly runs `$QMAKE -r -spec ... <pro> [extras]` with an
# indeterminate (spinner) progress display. The qmake step doesn't
# get a determinate bar because a meaningful denominator is
# expensive to compute -- the cheap candidates (disk `.pro` count)
# over-count, and the accurate one (recursive `qmake -E` walk) takes
# minutes on big trees and felt like a hang to users. The spinner
# is enough to show qmake is alive; bh_make's bar will show
# determinate progress for the work that follows.
#
# See also docs of: bh_run, for the verbose / non-tty / --no-progress
# path behavior (tee in verbose, silent log + dump-on-failure otherwise)
# that this function mirrors.
#
# The qmake invocation always carries `-r`. Without `-r` qmake only
# writes the top-level dispatcher Makefile and defers per-subdir
# qmake to make-time, which would surface as a flood of qmake
# work happening silently inside the make step.
bh_run_qmake() {
    _name=$1; _pro=$2; shift 2
    # Ensure $QMAKE is set (idempotent; bootstraps qmake if missing). This
    # used to be a side-effect of the always-on TARGET-resolve step, but
    # that's now lazy (skipped unless --test/--run), so the qmake step
    # must guarantee it for itself rather than depend on an earlier call.
    bh_qmake_prepare
    _step=$((_step + 1))
    _log=$LOGS/$(printf '%02d-%s.log' "$_step" "$_name")
    case "$_name" in
        qmake)         _label=root-project ;;
        test-qmake)    _label=tests ;;
        *-qmake)       _label=${_name%-qmake} ;;
        *)             _label=$_name ;;
    esac


    # Skip qmake when its output would be a no-op: Makefile in the
    # cwd already exists and is fresher than every .pro / .pri /
    # .prf file under the source tree. `--qmake` / `--refresh`
    # force a re-run; a missing Makefile (post-`--wipe` or first
    # build) also forces one. `--ignore-pri` narrows the freshness
    # check back to just `.pro` files for cases where an
    # unchanged-in-effect `.pri` keeps tripping a re-qmake
    # (e.g. an IDE that `touch`-es a .pri on save).
    if [ "${BH_FORCE_QMAKE:-0}" -eq 0 ] && [ -f Makefile ]; then
        # `|| true`: `head -1` closes the pipe after the first match, so
        # `find` is killed by SIGPIPE and pipefail (best-effort in
        # bh_strict_mode) would surface 141 -- that must not abort this
        # harmless freshness probe when many files are newer than the
        # Makefile (e.g. a fresh checkout or a bulk touch). Same idiom as
        # the `make -n | grep -c || true` precount below.
        if [ "${BH_IGNORE_PRI:-0}" -eq 0 ]; then
            _bhq_newer=$(find "$BH_ROOT" \
                \( -name '*.pro' -o -name '*.pri' -o -name '*.prf' \) \
                ! -path '*/build/*' ! -path '*/.git/*' \
                -newer Makefile 2>/dev/null | head -1 || true)
        else
            _bhq_newer=$(find "$BH_ROOT" -name '*.pro' \
                ! -path '*/build/*' ! -path '*/.git/*' \
                -newer Makefile 2>/dev/null | head -1 || true)
        fi
        if [ -z "$_bhq_newer" ]; then
            # Treat the skip as a successful step: show the row
            # with the checkmark + description so the user can see
            # that qmake decided to skip.
            bh_progress_setType spinner
            bh_progress_setMax 0
            bh_progress_setDescription "QMake skipped for $_label (Makefile is up-to-date, pass --qmake to force rerun it)."
            bh_progress_hide
            return 0
        fi
    fi

    # qmake's recursive run prints a "Reading <pro> [<dir>]" line per
    # subdir -- a firehose on a big tree -- so it's gated behind -vv
    # (very-verbose, level 3); plain -v keeps it in the log.
    if [ "${BH_LOG_LEVEL:-1}" -ge 3 ]; then
        "$QMAKE" -r -spec "$XD_DIR/mkspecs/$QMAKESPEC" "$_pro" "$@" 2>&1 \
            | tee "$_log"
        return $?
    fi

    if bh_progress_active; then
        bh_progress_setType spinner
        bh_progress_setMax 0
        bh_progress_setDescription "QMake is generating Makefile(s) for $_label."
        "$QMAKE" -r -spec "$XD_DIR/mkspecs/$QMAKESPEC" "$_pro" "$@" \
            >"$_log" 2>&1 &
        _pid=$!
        bh_progress_render                    # initial frame
        bh_spin_until "$_pid"
        _status=0
        wait "$_pid" || _status=$?
        if [ "$_status" -eq 0 ]; then
            bh_progress_hide
        else
            bh_progress_hide X
            bh_dump_fail "$_log" "$_status"
        fi
    else
        if "$QMAKE" -r -spec "$XD_DIR/mkspecs/$QMAKESPEC" "$_pro" "$@" \
            >"$_log" 2>&1; then
            :
        else
            _status=$?
            bh_dump_fail "$_log" "$_status"
        fi
    fi
}


# ---- build-dir lifecycle -----------------------------------------------

# bh_wipe
#
# `rm -rf $BUILD_DIR`. The caller has signed up by passing `--wipe`,
# so there's no confirmation prompt -- we just print what we're
# wiping. No-op when the directory doesn't exist.
#
# Run BEFORE `bh_logs_init` (which would recreate the dir as a
# side-effect of `mkdir -p`) so the post-wipe state is genuinely
# empty; the build flow then rebuilds from scratch.
bh_wipe() {
    [ -n "${BUILD_DIR:-}" ] || bh_error "bh_wipe: BUILD_DIR not set"
    # `bh_wipe` is *uncounted*: it runs before bh_logs_init so
    # `_step` is still unset, which makes the spinner row read
    # `[/]: Deleting ...` (no `(N/M)` between `]` and `:`).
    # Treating --wipe as a counted phase would push every
    # subsequent step number up by one without giving the user a
    # meaningful prep-vs-build split -- delete is housekeeping,
    # the actual build pipeline starts at Resolving.
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Skipping --wipe: $BUILD_DIR does not exist"
        return 0
    fi
    # `rm -rf` on a large build tree can take several seconds with no
    # visible activity, so animate a spinner while it runs (no (N/M) --
    # see the uncounted-phase note above).
    # See also docs of: bh_spin_until, for the background-poll spinner.
    if bh_progress_active; then
        bh_progress_setType spinner
        bh_progress_setMax 0
        bh_progress_setDescription "Deleting $BUILD_DIR"
        rm -rf "$BUILD_DIR" &
        _bh_wipe_pid=$!
        bh_progress_render
        bh_spin_until "$_bh_wipe_pid"
        wait "$_bh_wipe_pid" 2>/dev/null || true
        bh_progress_hide
    else
        echo "Deleting $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi
}

# bh_clean
#
# `make clean` inside $BUILD_DIR -- the Makefile's own clean target.
# Lighter than `--wipe`: removes object files and generated headers
# but keeps the final binary and the Makefile itself, so the next
# qmake pass is a no-op rather than a full regen.
#
# Best-effort. `make -k clean` keeps going past sub-Makefiles that
# lack a `clean` target (e.g. XD's qmake bootstrap dir, which is
# built by a hand-rolled Makefile.unix that doesn't define one), and
# the trailing `|| true` swallows the non-zero exit so a single
# missing target doesn't abort the whole wipe. The log
# (`$LOGS/<NN>-clean.log`) still captures everything for inspection.
# No-op when there's no Makefile (fresh checkout, or post-`--wipe`).
#
# Requires bh_logs_init to have run first (we manage $LOGS / $_step
# directly here rather than via bh_run, so we can ignore the exit).
#
# $BH_CLEAN_EXTRA (optional): a whitespace/glob list of *auto-generated*
# files outside the build dir that a fresh clone wouldn't carry -- e.g.
# the generated `mkspecs/modules/static/qt_lib_*.pri`. A normal rebuild
# must keep these (consumers read them), so they're only removed under
# `--clean`, letting `--clean` simulate out-of-the-box state without a
# real re-clone (which would also drop submodules like OpenSSL). Because
# they're qmake *inputs*, removing them forces a qmake re-run so they're
# regenerated this same build.
bh_clean() {
    [ -n "${BUILD_DIR:-}" ] || bh_error "bh_clean: BUILD_DIR not set"
    if [ -n "${BH_CLEAN_EXTRA:-}" ]; then
        echo "  removing auto-generated: $BH_CLEAN_EXTRA" 1>&2
        # shellcheck disable=SC2086  # intentional glob + word-split.
        rm -f $BH_CLEAN_EXTRA
        BH_FORCE_QMAKE=1   # regenerate what we just removed
    fi
    if [ -f "$BUILD_DIR/Makefile" ]; then
        _step=$((_step + 1))
        _log=$LOGS/$(printf '%02d-%s.log' "$_step" clean)
        echo "  make -k clean in $BUILD_DIR" 1>&2
        if ! make -C "$BUILD_DIR" -k clean >"$_log" 2>&1; then
            echo "  (note: --clean had partial failures; see $_log)" 1>&2
        fi
    else
        echo "Skipping --clean: no Makefile in $BUILD_DIR"
    fi
}


# ---- race-aware make wrapper -------------------------------------------

# bh_make <jobs> [target...]
#
# Plain `make -j$JOBS` races between `release-all` and `debug-all` when
# a debug_and_release subproject emits its artifact into the build-dir
# root and then `mv`s the file into the per-config DESTDIR -- both
# configs collide on the same intermediate filename and one mv ends up
# failing because the file was already moved by the other config (the
# exact failure pattern XD's OpenSSL 1.0.1c subprojects, libeay32 +
# ssleay32, hit on macOS / Linux).
#
# When the generated Makefile exposes both top-level targets, run them
# sequentially -- each sub-make still parallelises internally via the
# passed <jobs>, so the only thing serialised is the per-config link
# / mv step. Otherwise fall back to a single `make`.
bh_make() {
    _bh_jobs=$1; shift
    # Race-aware detection: cheap grep beats `make -n <tgt>` which
    # would re-evaluate the full Makefile (seconds on a big tree).
    if [ -f Makefile ] \
        && grep -q '^release-all:' Makefile \
        && grep -q '^debug-all:'   Makefile
    then
        # The split makes two bh_make_step calls instead of one,
        # and each bh_make_step contributes two visible phases
        # (computing + compiling) -- so the split adds two extra
        # phases overall (release-computing + release-compiling
        # vs. the single non-split pair). Bump BH_STEPS_TOTAL by 2
        # so the counter walks all the way to N/N at the end of
        # `make-debug compiling`. Templates seed assuming a single
        # non-split bh_make.
        if [ -n "${BH_STEPS_TOTAL:-}" ]; then
            BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 2))
        fi
        bh_make_step make-release  release-all "$_bh_jobs" "$@"
        bh_make_step make-debug    debug-all   "$_bh_jobs" "$@"
    else
        bh_make_step make ""        "$_bh_jobs" "$@"
    fi
}

# bh_make_step <log-name> <target-or-empty> <jobs> [extra-make-args...]
#
# Internal driver for one `make` invocation with a live progress bar.
# Prefer `bh_make` for the top-level build (it auto-handles the
# debug/release race-aware split); use this directly for one-off
# sub-makes that don't need the split (test-make, helper builds).
#
# Bar mechanics. The denominator comes from a pre-flight `make -n` (dry
# run); the numerator advances per build-step line during the real run --
# both counted with the same pattern. Progress goes through the shared
# `bh_progress_*` API; awk is only a stream filter (emit one-letter
# tokens) so the shell loop has minimal per-line overhead. Each
# `bh_progress_setMax` resets value to 0, so the bar resets cleanly
# between phases.
#
# See also docs of:
#   - BH_BUILD_STEP_RE, for which command lines count as build steps and
#     why recursive driver lines (`cd X/ && make ...`) are excluded.
#   - bh_run, for the shared output handling (tee to
#     `$LOGS/NN-<log-name>.log`, verbose tee-to-terminal, dump-on-failure);
#     the only addition here is the clear-line before each scrolled
#     verbose line so the bar stays pinned at the bottom. Use bh_run
#     directly for steps where progress doesn't matter.
bh_make_step() {
    _bh_name=$1; _bh_tgt=$2; _bh_jobs=$3; shift 3

    # Phase 1 of two: "computing build plan". Advances the
    # `(step/total)` counter by one and shows a rotating spinner while
    # `make -n` runs in the background (its textual output redirected to
    # a temp file the user never sees).
    # See also docs of: bh_spin_until, for the background-poll spinner.
    _step=$((_step + 1))
    bh_progress_setType spinner
    bh_progress_setMax 0
    bh_progress_setDescription "Makefile runner ($_bh_name) is computing build plan."
    bh_progress_render                       # initial frame so the row appears

    if bh_progress_active; then
        _bh_precount=$LOGS/$(printf '%02d-%s.precount.tmp' "$_step" "$_bh_name")
        if [ -n "$_bh_tgt" ]; then
            ( make -n "$_bh_tgt" "$@" 2>/dev/null > "$_bh_precount" ) &
        else
            ( make -n "$@" 2>/dev/null > "$_bh_precount" ) &
        fi
        _bh_pid=$!
        bh_spin_until "$_bh_pid"
        wait "$_bh_pid" 2>/dev/null || true
        _bh_total=$(grep -cE "$BH_BUILD_STEP_RE" \
            "$_bh_precount" 2>/dev/null || true)
        rm -f "$_bh_precount"
    else
        # Headless / non-TTY: synchronous precount, no spinner.
        # `grep -c` exits 1 on zero matches; `|| true` keeps
        # pipefail+errexit quiet (grep still prints "0").
        if [ -n "$_bh_tgt" ]; then
            _bh_total=$(make -n "$_bh_tgt" "$@" 2>/dev/null \
                | grep -cE "$BH_BUILD_STEP_RE" || true)
        else
            _bh_total=$(make -n "$@" 2>/dev/null \
                | grep -cE "$BH_BUILD_STEP_RE" || true)
        fi
    fi
    bh_progress_hide                         # row settles on [√] (N/M)

    # Phase 2 of two: "compiling source-codes". A separate step --
    # increment again so the counter walks (qmake -> computing
    # -> compiling), and pick a log slot that matches the step
    # number for the actual make output.
    _step=$((_step + 1))
    _bh_log=$LOGS/$(printf '%02d-%s.log' "$_step" "$_bh_name")
    # Track whether this is a no-op build (make -n found zero
    # compile commands). When there's no computable progress to
    # display, we switch to spinner mode rather than show a fake
    # `(0/1) 0%` bar that wouldn't move -- spinner mode is the
    # rule for "progress is unknown / indeterminate"; the bar (with
    # its embedded leading-edge spinner) is reserved for "progress
    # is known and bounded".
    _bh_no_work=0
    if [ "${_bh_total:-0}" -le 0 ]; then
        _bh_total=1
        _bh_no_work=1
    fi

    if [ "$_bh_no_work" -eq 1 ]; then
        bh_progress_setType spinner
        bh_progress_setMax 0
    else
        bh_progress_setType bar
        bh_progress_setMax "$_bh_total"
    fi
    bh_progress_setDescription "Makefile runner ($_bh_name) is compiling source-codes."
    bh_progress_render                # row appears immediately so
                                      # there's no silent gap between
                                      # the description header (bar)
                                      # / inline desc (spinner) and the
                                      # first compile-line render.

    # Disable `set -e` for the pipeline so we can inspect $? ourselves
    # and dump the log on failure -- pipefail (best-effort in
    # bh_strict_mode) propagates the leftmost non-zero status, so we
    # see the make exit code, not awk's.
    set +e
    {
        # Keepalive ticker: writes "KEEPALIVE" to stdout every
        # BH_SPIN_INTERVAL so the embedded spinner advances even when
        # compile events are sparse (slow link, heavy template
        # instantiation, low CPU contention -- the bar would
        # otherwise sit frozen on its current spinner frame for
        # the whole stall). The ticker runs alongside make inside
        # this brace group; its output is injected AFTER `tee`,
        # so the .log file stays clean of KEEPALIVE markers.
        # Killed on make exit; SIGPIPE from the awk-closed pipe is
        # the backup exit path if kill races the sleep boundary.
        ( while sleep "$BH_SPIN_INTERVAL"; do printf 'KEEPALIVE\n' || exit 0; done ) &
        _bh_ticker_pid=$!
        if [ -n "$_bh_tgt" ]; then
            make "$_bh_tgt" -j"$_bh_jobs" "$@" 2>&1 | tee "$_bh_log"
        else
            make -j"$_bh_jobs" "$@" 2>&1 | tee "$_bh_log"
        fi
        _bh_make_st=$?
        kill "$_bh_ticker_pid" 2>/dev/null || true
        wait "$_bh_ticker_pid" 2>/dev/null || true
        exit "$_bh_make_st"
    } | awk -v verbose="${BH_LOG_LEVEL:-1}" '
            # Keepalive marker -> "K" sentinel (spinner tick, no
            # VALUE change). Must come before the catch-all so
            # verbose mode does not log "KEEPALIVE" as a build line.
            /^KEEPALIVE$/ { print "K"; fflush(); next }
            # Compile/link lines -> "P" sentinel (one per step). Same
            # BH_BUILD_STEP_RE as the precount, read from the exported
            # environment (ENVIRON) rather than `-v` so its `\\` and
            # `[+]`/`[.]` survive the awk value-escape step. Verbose
            # mode also forwards the original line with an "L " prefix
            # so the shell loop can clear-line-then-print. `fflush()`
            # defeats awk pipe-buffering -- without it, awk accumulates
            # ~kB of output before the shell loop gets to read, so the
            # bar appears to lag behind make.
            $0 ~ ENVIRON["BH_BUILD_STEP_RE"] {
                print "P"
                if (verbose+0 >= 2) print "L " $0
                fflush()
                next
            }
            verbose+0 >= 2 { print "L " $0; fflush() }
        ' | (
            # Subshell: BH_PROGRESS_MAX inherits; VALUE updates stay
            # local (which is fine -- renders happen here, the parent
            # only needs the post-pipeline hide).
            while IFS= read -r _bhm_line; do
                case "$_bhm_line" in
                    P)
                        # Render on every compile-line. Earlier
                        # versions throttled to per-integer-% which
                        # made big trees (XD, 2k+ steps) feel stuck
                        # because each redraw covered ~25 compiles
                        # = ~30s of real time. Pure-shell render is
                        # ~ms-fast, the rate is bounded by compile
                        # throughput anyway.
                        BH_PROGRESS_VALUE=$((BH_PROGRESS_VALUE + 1))
                        bh_progress_render
                        ;;
                    "L "*)
                        # Clear the bar's row, print the verbose
                        # line on it, then re-render the bar on
                        # the next row -- visually pins the bar
                        # below the scrolling stream.
                        printf '\r\033[K%s\n' "${_bhm_line#L }"
                        bh_progress_render
                        ;;
                    K)
                        # Keepalive: just tick the spinner. No
                        # VALUE change (no compile actually
                        # happened), but bh_progress_render bumps
                        # BH_PROGRESS_TICK so the |/-\ frame
                        # advances -- the fix for "embedded
                        # spinner hangs at low CPU".
                        bh_progress_render
                        ;;
                esac
            done
        )
    _bh_status=$?
    set -e
    # The pipeline's `while read` ran in a subshell, so the
    # BH_PROGRESS_VALUE increments per compile-line never reached
    # this (parent) shell -- parent's VALUE is still 0. Snap it to
    # MAX on success so `bh_progress_hide` renders the bar row's
    # final state as `[√] (N/N)` instead of `[√] (0/N)`.
    if [ "$_bh_status" -eq 0 ] && [ "$_bh_no_work" -eq 0 ]; then
        BH_PROGRESS_VALUE=$BH_PROGRESS_MAX
    fi
    if [ "$_bh_status" -eq 0 ]; then
        bh_progress_hide              # replaces row with [√] + (V/M) + \n
    else
        bh_progress_hide X            # [X] on make failure
        bh_dump_fail "$_bh_log" "$_bh_status"
    fi
}


# ---- qmake availability + thin wrapper ---------------------------------

# bh_qmake_prepare
#
# Ensures $QMAKE points at a working qmake binary. The XD-tracked
# binary at $XD_DIR/bin/qmake is the canonical answer; when it's
# missing, we offer to build it via $XD_DIR/qmake/build.sh.
#
# Under --headless we skip the prompt and just build (matches the
# Xcode-license behavior elsewhere -- headless = no interactive
# blocking).
#
# Caller-set state used here:
#   $XD_DIR        framework root that owns qmake/
#   $BUILD_DIR     where the qmake bootstrap's intermediate objects
#                  land (the framework's own build.sh uses
#                  $XD_BUILD_DIR; we fall back to that when $BUILD_DIR
#                  isn't set).
#   $BH_HEADLESS   non-zero auto-builds without prompting.
bh_qmake_prepare() {
    # Pick the platform's qmake binary. bin/qmake itself is only the dispatch
    # script (it forwards to one of these by host); we resolve the real binary
    # here so the bootstrap-if-missing check below tests the actual executable.
    # All three host binaries are committed -- bin/qmake-macos (Mach-O),
    # bin/qmake.exe (PE), bin/qmake-linux (ELF) -- and qmake/build.sh rebuilds
    # the host's on demand should it go missing, each under its own name so no
    # build overwrites another's.
    case "$(uname -s)" in
        Darwin)               QMAKE=$XD_DIR/bin/qmake-macos ;;
        MINGW*|MSYS*|CYGWIN*) QMAKE=$XD_DIR/bin/qmake.exe ;;
        *)                    QMAKE=$XD_DIR/bin/qmake-linux ;;
    esac
    [ -x "$QMAKE" ] && return 0
    echo
    printf 'XD qmake not found at "%s".\n' "$QMAKE"
    bh_yes_no "Build it now via \"$XD_DIR/qmake/build.sh\"? [Y/n]" \
        || bh_error "XD qmake not found at \"$QMAKE\" -- aborting"
    echo
    _bh_intermediate=${BUILD_DIR:-${XD_BUILD_DIR:-$XD_DIR/build}}/qmake
    bh_run bootstrap-qmake sh "$XD_DIR/qmake/build.sh" -d "$_bh_intermediate"
    [ -x "$QMAKE" ] || bh_error "qmake build at \"$XD_DIR/qmake/build.sh\" did not produce \"$QMAKE\""
}

# bh_qmake <args...>
#
# Invokes qmake in evaluation mode (`-E`) with the project's standard
# `-spec $XD_DIR/mkspecs/$QMAKESPEC` + `CONFIG+=$BH_MODE` baked in,
# and qmake's stderr suppressed (`2>/dev/null`) so the caller's
# downstream awk/grep doesn't have to deal with the warning noise an
# unfocused `-E` run emits.
#
# Caller passes the .pro path -- plus any follow-up assignments
# qmake should evaluate (extra `CONFIG+=...`, `TASK=...`,
# `ARG1=...`) -- as <args>; everything is appended after the fixed
# prefix:
#
#     "$QMAKE" -E -spec ".../<spec>" CONFIG+="$BH_MODE" <args> 2>/dev/null
#
# bh_qmake_prepare runs first so qmake is always ready.
bh_qmake() {
    bh_qmake_prepare
    "$QMAKE" -E -spec "$XD_DIR/mkspecs/$QMAKESPEC" \
        CONFIG+="${BH_MODE:-debug}" "$@" 2>/dev/null
}

# bh_qmake_task <task> [arg1] [arg2] [arg3] [arg4] [arg5]
#
# Shell-side mirror of `xd_taskHandlerRun` from
# `mkspecs/features/xd_functions.prf`. Invokes qmake with
# `XD/tools/task-handler/task-handler.pro` so the same TASK=...
# ARG1=... ARG2=... pattern that .pro files use is reachable from
# build.sh scripts -- handy for the rare cases where qmake's `-E`
# can't reveal a derived variable (the task-handler can also be
# extended with new TASK names without changing this wrapper).
#
# Currently unused by the rest of the handler (bh_parse_target gets
# what it needs straight from `bh_qmake -E`); shipped here so future
# callers don't have to re-derive the right invocation shape.
bh_qmake_task() {
    _bh_task=$1; shift
    bh_qmake "$XD_DIR/tools/task-handler/task-handler.pro" \
        "CONFIG+=force_only_debug" \
        TASK="$_bh_task" \
        ARG1="${1:-}" \
        ARG2="${2:-}" \
        ARG3="${3:-}" \
        ARG4="${4:-}" \
        ARG5="${5:-}" \
        -o -
}

# ---- target resolution + launch ----------------------------------------

# bh_parse_target <pro_path>
#
# Derives the app's target name (the executable basename qmake will
# produce) from the project file. We evaluate the .pro via
# `qmake -E ...` and read the resolved TARGET; that handles the case
# where TARGET is set inside an included `.pri` (e.g. a
# `tests/test.pro` picks up `TARGET = <App>-test-runner` from
# `config/test-mode.pri`) rather than guessing from the .pro filename.
#
# The basename-of-.pro fallback covers the rare case where qmake -E
# can't evaluate cleanly (the basename matches the conventional Qt
# app naming for top-level `<App>.pro` files anyway).
#
# The result lives in BH_TARGET_NAME and is consumed by the test
# step (`<BH_TARGET_NAME>-test-runner`) and by `bh_run_target`.
#
# Normally invoked lazily via bh_target_resolve (below), not directly.
bh_parse_target() {
    _pro=$1
    # Pre-run bh_qmake_prepare in *this* shell so $QMAKE survives. The
    # bh_qmake call below runs inside a `$(...)` substitution; any
    # variable bh_qmake_prepare assigns inside that subshell would be
    # discarded when the substitution closes, and later bh_run-qmake
    # callers would see $QMAKE expand to empty (sh's "command not found"
    # on a blank command).
    bh_qmake_prepare
    _bh_target=$(bh_qmake "$_pro" \
                 | awk -F' = ' '/^TARGET / {print $2; exit}')
    if [ -n "$_bh_target" ]; then
        BH_TARGET_NAME=$_bh_target
    else
        _bh_base=$(basename "$_pro")
        BH_TARGET_NAME=${_bh_base%.pro}
    fi
}

# bh_target_resolve <pro_path>
#
# Lazy fetch of the project's TARGET name. Only `--test` / `--run` need
# it, so bh_parse_args marks BH_TARGET_NAME=pending for those flags;
# this runs the "Resolving project TARGET name" step (a ~1s `qmake -E`)
# only when that sentinel is present, then caches the real name back
# into BH_TARGET_NAME. A plain build never marks it pending, so it skips
# the step entirely. Future flags that need the name just have
# bh_parse_args mark BH_TARGET_NAME=pending; a caller may instead
# pre-set BH_TARGET_NAME to a literal to skip resolution altogether.
bh_target_resolve() {
    [ "${BH_TARGET_NAME:-}" = pending ] || return 0
    BH_TARGET_NAME=   # drop the sentinel so bh_parse_target sets the real one
    bh_run_silent_fg 'Resolving project TARGET name (qmake -E)' \
        bh_parse_target "$1"
}

# bh_run_target
#
# When BH_RUN_TARGET is set (i.e. the user passed `--run`), boots the
# freshly-built target -- the executable named `$BH_TARGET_NAME` --
# keeping stdout/stderr attached to this terminal so `qDebug()` output
# is visible.
#
# On macOS the build produces a `<TARGET>.app` bundle; we exec the
# inner binary rather than handing the bundle to `open`, because XD's
# dylibs ship with bare-name install paths (no @rpath, no absolute
# prefix) and `open` routes through launchd which strips DYLD_* env
# vars for security -- the app would launch, fail to dlopen its Qt
# libs, and die silently. The direct-exec path lets us inject
# DYLD_LIBRARY_PATH and Qt's plugin-search vars so the cocoa platform
# plugin and the linked dylibs all resolve.
#
# Assumes $BUILD_DIR, $XD_DIR, $QT_PLUGINS_STAGE are set (i.e. you've
# either called bh_template_app or set them up yourself).
bh_run_target() {
    [ "${BH_RUN_TARGET:-0}" -gt 0 ] || return 0
    if [ -x "$BUILD_DIR/bin/$BH_TARGET_NAME.app/Contents/MacOS/$BH_TARGET_NAME" ]; then
        _bin=$BUILD_DIR/bin/$BH_TARGET_NAME.app/Contents/MacOS/$BH_TARGET_NAME
    elif [ -x "$BUILD_DIR/bin/$BH_TARGET_NAME" ]; then
        _bin=$BUILD_DIR/bin/$BH_TARGET_NAME
    elif [ -x "$BUILD_DIR/bin/$BH_TARGET_NAME.exe" ]; then
        _bin=$BUILD_DIR/bin/$BH_TARGET_NAME.exe
    else
        bh_error "no $BH_TARGET_NAME executable found under \"$BUILD_DIR/bin\""
    fi
    echo "Launching $_bin"
    DYLD_LIBRARY_PATH=$XD_DIR/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH} \
    LD_LIBRARY_PATH=$XD_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
    QT_PLUGIN_PATH=$QT_PLUGINS_STAGE${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH} \
    QT_QPA_PLATFORM_PLUGIN_PATH=$QT_PLUGINS_STAGE/platforms \
    "$_bin"
}


# ---- test orchestration -----------------------------------------------

# bh_run_tests <target> [--subdirs]
#
# Drives the `--test` step for two test-project shapes:
#
# 1) Single testcase app (single-app style):
#    `tests/test.pro` is `TEMPLATE = app` with `CONFIG += testcase`.
#    Pass <target> = absolute path to the built test binary. We exec it
#    directly: the caller has already resolved that path and exported the
#    runtime env (DYLD_LIBRARY_PATH / QT_PLUGIN_PATH), so there's no need
#    to drive a single binary through `make check` and qmake's generated
#    `target_wrapper.sh` (which just sets the same env, then execs it).
#
#    Different consumers install the runner in different layouts
#    (next to the host `.app`, inside `Contents/MacOS/`, flat under
#    `bin/`), so we let the caller resolve the exact path rather
#    than hard-coding a search list here.
#
# 2) Subdirs of testcase apps (the consumer's $BH_TEST_ROOT pointing at
#    a `TEMPLATE = subdirs` .pro that recurses through many testcase apps):
#    Pass <target> = that subdirs project's build dir and add `--subdirs`.
#    `make check` is the canonical run -- qmake emits the recursive
#    driver that walks each subdir's testcase. Caller must `cd` to
#    <target> first (matches the bh_make / bh_run convention).
bh_run_tests() {
    _target=$1
    case "${2:-}" in
        --subdirs)
            bh_run tests make -k check
            ;;
        *)
            [ -x "$_target" ] || bh_error "test runner not found at '$_target'"
            bh_run tests "$_target"
            ;;
    esac
}

# bh_run_one_test <abs-pro>
#
# Builds and runs one test project, in its own build and log subdir (so
# parallel sweep entries don't clobber each other), and tallies the
# outcome into the caller's _bh_tests_total / _bh_tests_failed. Runs the
# build+run in a subshell so one failure stays contained (bh_dump_fail's
# `exit`, or a propagated non-zero, ends only the subshell) and the sweep
# goes on. Steps are chained with && so the first failure short-circuits
# the subshell to a non-zero exit -- we do not lean on `set -e`, which
# bash leaves disabled (and won't reliably re-arm) inside a subshell used
# as a condition; `|| _rc=$?` captures it without tripping the outer -e.
# `make check` recurses for a subdirs root and runs the one check rule
# for a single testcase app (testcase.prf invokes qtAddTargetEnv's
# ./target_wrapper.sh -- see qt_functions.prf).
bh_run_one_test() {
    _bhrot_pro=$1
    [ -f "$_bhrot_pro" ] || bh_error "test project not found: $_bhrot_pro"
    _bh_tests_total=$((_bh_tests_total + 1))
    # Flat, unique name for this test's build + log subdirs.
    _bhrot_name=$(printf '%s' "${_bhrot_pro#"$BH_ROOT"/}" | tr -c 'A-Za-z0-9' '_')
    _bhrot_rc=0
    # shellcheck disable=SC2086  # intentional split of forwarded args.
    (
        LOGS=$BUILD_DIR/logs/tests/$_bhrot_name
        _bhrot_tb=$BUILD_DIR/tests-build/$_bhrot_name
        _step=0
        if [ "${BH_BUILD_TESTS:-1}" -eq 0 ]; then
            # Don't (re)build the test -- assume it's already built and
            # just run it. `make check` on an up-to-date tree only runs the
            # test; a missing build makes it fail, the "fail if not really
            # built" contract.
            [ -e "$_bhrot_tb/Makefile" ] || bh_error \
                "test '$_bhrot_name' isn't built yet (build it, or pass --build-tests)"
            mkdir -p "$LOGS" \
                && cd "$_bhrot_tb" \
                && bh_run_tests "$_bhrot_tb" --subdirs
        else
            mkdir -p "$LOGS" "$_bhrot_tb" \
                && cd "$_bhrot_tb" \
                && bh_run_qmake test-qmake "$_bhrot_pro" $BH_REMAINING_ARGS \
                && bh_make "$JOBS" \
                && bh_run_tests "$_bhrot_tb" --subdirs
        fi
    ) || _bhrot_rc=$?
    [ "$_bhrot_rc" -eq 0 ] || _bh_tests_failed=$((_bh_tests_failed + 1))
}


# ---- template helpers --------------------------------------------------

# bh_searchpath_plugins
#
# Adjusts where Qt finds plugins at run/test time. Builds a per-mode
# tree under `$BUILD_DIR/qt-plugins-$BH_MODE/` populated with only the
# matching-variant plugin files, and exports `$QT_PLUGINS_STAGE`
# pointing at it; later launch/test steps wire `QT_PLUGIN_PATH` and
# `QT_QPA_PLATFORM_PLUGIN_PATH` from that variable. Skipped entirely
# when `BH_SKIP_PLUGIN_STAGE=1` is set.
#
# Why per-mode rather than just pointing at `$XD_DIR/plugins/`: XD
# ships both debug and release copies of every plugin (e.g. on macOS
# `libqcocoa.dylib` *and* `libqcocoa_debug.dylib`; matching pairs on
# Linux/Windows with the respective extension). `QFactoryLoader` walks
# the entire search directory and dlopens every candidate just to read
# its metadata, and the dlopen pulls in the plugin's linked Qt5*
# library variant. On macOS that's the most explosive case: the
# plugin's Objective-C `+load` methods run during dlopen, duplicate
# class registrations clash ("Class RunLoopModeTracker is implemented
# in both ..."), and the app segfaults during `QObject::moveToThread`.
# Linux and Windows show subtler symptoms but the same per-mode tree
# fixes them all.
#
# Cross-platform notes:
#   - Library extension is picked by host (`.dylib` on macOS, `.so`
#     on Linux/BSD, `.dll` on Windows incl. git-bash/MSYS/Cygwin);
#     the `_debug` filename suffix is XD's convention everywhere (see
#     `xd_suffix` in `mkspecs/features/xd_functions.prf`).
#   - `ln -sf` is a real symlink on macOS/Linux and silently degrades
#     to a plain file copy under MSYS / git-bash (no native symlink
#     support without admin rights). The end state -- a plugin file
#     findable under the staged tree -- is identical either way.
#
# Timing differs by template:
#   - `bh_template_app` calls this BEFORE make (consumer apps consume
#     XD's already-built plugins).
#   - `bh_template_subdirs` calls it AFTER make so a project that
#     BUILDS its own plugins (XD itself) actually has files to point
#     the search path at.
bh_searchpath_plugins() {
    [ "${BH_SKIP_PLUGIN_STAGE:-0}" -eq 0 ] || return 0
    case "$(uname -s)" in
        Darwin)                _bh_ext=dylib ;;
        CYGWIN*|MINGW*|MSYS*)  _bh_ext=dll ;;
        *)                     _bh_ext=so ;;
    esac
    QT_PLUGINS_STAGE=$BUILD_DIR/qt-plugins-$BH_MODE
    rm -rf "$QT_PLUGINS_STAGE"
    for _bh_src_dir in "$XD_DIR/plugins"/*/; do
        [ -d "$_bh_src_dir" ] || continue
        _type=$(basename "$_bh_src_dir")
        _dest=$QT_PLUGINS_STAGE/$_type
        mkdir -p "$_dest"
        for _src in "$_bh_src_dir"*."$_bh_ext"; do
            [ -f "$_src" ] || continue
            _base=$(basename "$_src")
            case "$BH_MODE" in
                debug)
                    case "$_base" in *_debug."$_bh_ext") ln -sf "$_src" "$_dest/$_base";; esac;;
                release)
                    case "$_base" in *_debug."$_bh_ext") ;; *) ln -sf "$_src" "$_dest/$_base";; esac;;
            esac
        done
    done
}


# ---- one-stop driver for app consumers --------------------------------

# bh_template_app <pro_path> [argv...]
#
# All-in-one driver for a single-app Qt project. The consumer's
# build.sh only resolves XD_DIR and points us at its .pro; everything
# else (arg parsing, build dir, plugin staging, qmake, make, tests,
# launch) lives here. Use this as the template for new app repos --
# XD itself is the exception because it builds the framework, not a
# consumer.
#
# Convention:
#   - The app's executable name matches the .pro basename
#     (`MyApp.pro` → `MyApp`).
#   - Test runner (when `--test` runs) is named
#     `<app-target>-test-runner` and lives in the install dir; for
#     macOS we also look inside the host app bundle's `Contents/MacOS/`.
#   - Test project lives at `<repo>/tests/test.pro` (with a fallback
#     to the legacy `<repo>/test/test.pro` for not-yet-renamed repos).
bh_template_app() {
    _pro=$1; shift
    BH_TEMPLATE=app
    bh_parse_args "$@"
    bh_strict_mode
    bh_default_jobs
    bh_default_qmakespec
    bh_macos_developer_dir

    # Pre-compute phase total for the "(N/M)" tail of the progress
    # line. Each user-visible step (qmake, make, test-qmake,
    # test-make, tests, run) is one phase; helper sub-steps (e.g.
    # bh_make's debug/release split) inflate the counter slightly
    # beyond the total -- the renderer just shows whatever (_step,
    # BH_STEPS_TOTAL) is at the moment, accepting small drift.
    # Base: bh_run_silent_fg (Staging) + qmake + make-computing +
    # make-compiling = 4. The "Resolving TARGET name" step is added below
    # only for --test/--run (the only consumers of BH_TARGET_NAME); a
    # plain build skips it. --wipe is *not* counted -- see also docs of:
    # bh_wipe, for why Deleting is an uncounted pre-build phase.
    BH_STEPS_TOTAL=4
    [ "${BH_CLEAN:-0}"       -gt 0 ] && BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 1))
    # --test / --run share one "Resolving TARGET name" step.
    if bh_has_tasks; then
        BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 1))
    fi
    # `--test` adds: test-qmake + test-make-computing + test-make-compiling + tests = 4
    [ "${BH_RUN_TESTS:-0}"   -gt 0 ] && BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 4))
    [ "${BH_RUN_TARGET:-0}"  -gt 0 ] && BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 1))

    # Consumer must have resolved the XD framework before calling us.
    [ -n "${XD_DIR:-}" ] || bh_error "XD_DIR not set -- resolve the XD framework first"

    # Per-mode build dir lives as a sibling to the source root, under a
    # shared `build/` folder named after this repo plus the build mode.
    # Keeping the mode in the dir name lets debug and release artifacts
    # coexist without stomping each other (the underlying Makefile
    # carries different QMAKE_CXXFLAGS, library paths, and link names
    # per mode).
    : "${BUILD_DIR:=$(dirname "$BH_ROOT")/build/$(basename "$BH_ROOT")-$BH_MODE}"

    # Show + gate. Same license-accept dance XD's build.sh runs, so
    # both templates share one user-facing flow.
    echo "Build directory:"
    echo "  $BUILD_DIR"
    echo
    bh_license_gate

    # --wipe first, then bail early on `--wipe --no-build` so we
    # don't immediately recreate the empty dir via the mkdir below.
    if [ "${BH_WIPE:-0}" -gt 0 ]; then
        bh_wipe
        [ "${BH_NO_BUILD:-0}" -gt 0 ] && return 0
    fi

    mkdir -p "$BUILD_DIR"
    bh_logs_init "$BUILD_DIR/logs"

    # --clean uses bh_run so it needs $LOGS in place; after `--wipe`
    # there's no Makefile to clean so it prints a "skipping" notice.
    [ "${BH_CLEAN:-0}" -gt 0 ] && bh_clean

    # --no-build / --only-clean exit point: the user asked for the wipes
    # above and nothing more. Exceptions where there's still work to do:
    #   * `--qmake --no-build` regenerates Makefiles (qmake runs below).
    #   * `--no-build --test` / `--no-build --run` mean "the build is
    #     already done -- skip building, but still run the post-build
    #     action". A build that isn't really done then surfaces as a
    #     test/run failure rather than being silently rebuilt.
    bh_can_exit pre-build && return 0

    # cd into BUILD_DIR *before* qmake-via-bh_parse_target runs --
    # qmake's `-E` mode (which bh_parse_target uses to extract the
    # TARGET name) writes a `.qmake.stash` to its cwd as a side
    # effect. If we resolve TARGET from the source root, the stash
    # leaks into the repo and shows up under `git status`. Doing
    # the cd here pins every qmake invocation in this template to
    # BUILD_DIR, keeping the source tree clean.
    cd "$BUILD_DIR"

    # qmake gate + (lazy) TARGET resolution happen here -- both can
    # require `bh_run` (bootstrap-qmake) so LOGS must already be
    # initialised. bh_target_resolve runs `qmake -E` (~1 s) only when
    # --test/--run marked BH_TARGET_NAME pending; bh_searchpath_plugins
    # builds the per-mode symlink farm (~4 s for full Qt). Without
    # spinners these looked like the script had hung after the `[√]` of
    # the previous step, so each shows a spinner row.
    bh_target_resolve "$_pro"
    bh_run_silent_fg 'Staging Qt plugins for runtime search path' \
        bh_searchpath_plugins

    # Build the main app.
    if [ "${BH_NO_BUILD:-0}" -eq 0 ]; then
        # Normal build: (re)generate the Makefile, then compile.
        # shellcheck disable=SC2086  # intentional word-split of forwarded args.
        bh_run_qmake qmake "$_pro" CONFIG+="$BH_MODE" $BH_REMAINING_ARGS
        bh_make "$JOBS"
    elif [ "${BH_FORCE_QMAKE:-0}" -gt 0 ]; then
        # `--qmake --no-build`: regenerate the Makefile only, no compile.
        # shellcheck disable=SC2086  # intentional word-split of forwarded args.
        bh_run_qmake qmake "$_pro" CONFIG+="$BH_MODE" $BH_REMAINING_ARGS
        # With no post-build action requested, stop after qmake.
        bh_can_exit post-qmake && return 0
    fi
    # else: --no-build with --test/--run and no --qmake -- skip the main
    # build entirely and fall through to the post-build action below.

    # Tests (--test / --tests).
    if [ "$BH_RUN_TESTS" -gt 0 ]; then
        _bhtest_pro=""
        if [ -f "$BH_ROOT/tests/test.pro" ]; then
            _bhtest_pro=$BH_ROOT/tests/test.pro
        elif [ -f "$BH_ROOT/tests/tests.pro" ]; then
            _bhtest_pro=$BH_ROOT/tests/tests.pro
        elif [ -f "$BH_ROOT/test/test.pro" ]; then
            # Backward-compat for repos still on the singular folder.
            _bhtest_pro=$BH_ROOT/test/test.pro
        fi
        if [ -n "$_bhtest_pro" ]; then
            # The test runner is conventionally installed *inside* the
            # host app bundle's `Contents/MacOS/` so it can share Qt
            # plugins / resources -- assert the host bundle is on disk
            # before letting qmake/make populate paths under it.
            case "$(uname -s)" in
                Darwin)
                    _bhhost_bin=$BUILD_DIR/bin/$BH_TARGET_NAME.app/Contents/MacOS/$BH_TARGET_NAME
                    [ -x "$_bhhost_bin" ] || bh_error \
                        "root app bundle missing at \"$_bhhost_bin\" -- regular build must complete first"
                    ;;
            esac

            TEST_BUILD=$BUILD_DIR/tests
            if [ "${BH_BUILD_TESTS:-1}" -gt 0 ]; then
                mkdir -p "$TEST_BUILD"
                cd "$TEST_BUILD"
                bh_run_qmake test-qmake "$_bhtest_pro" CONFIG+="$BH_MODE"
                # bh_make_step (not bh_make) because the test project is
                # a single-config app -- no debug_and_release race to
                # serialise. Live progress bar on the way through.
                bh_make_step test-make "" "$JOBS"
            fi
            # When not building tests, the runner resolved below must
            # already exist (else bh_error there) -- the "fail if not
            # really built" contract.

            # Export the runtime env once -- the test runner and the
            # later --run step both want the same DYLD/plugin paths,
            # and exporting at this scope (rather than a subshell)
            # keeps `bh_run`'s _step counter intact across calls.
            export DYLD_LIBRARY_PATH=$XD_DIR/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}
            export LD_LIBRARY_PATH=$XD_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
            export QT_PLUGIN_PATH=$QT_PLUGINS_STAGE${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}
            export QT_QPA_PLATFORM_PLUGIN_PATH=$QT_PLUGINS_STAGE/platforms

            # `make check` would drive the runner through qmake's
            # generated `target_wrapper.sh` (which just sets the runtime
            # env, already exported above). We've resolved the runner's
            # exact path, so exec it directly through bh_run_tests instead,
            # so the step lands in $LOGS/NN-tests.log.
            _runner=$BH_TARGET_NAME-test-runner
            if [ -x "$BUILD_DIR/bin/$BH_TARGET_NAME.app/Contents/MacOS/$_runner" ]; then
                _bhtest_bin=$BUILD_DIR/bin/$BH_TARGET_NAME.app/Contents/MacOS/$_runner
            elif [ -x "$BUILD_DIR/bin/$_runner" ]; then
                _bhtest_bin=$BUILD_DIR/bin/$_runner
            elif [ -x "$BUILD_DIR/bin/$_runner.exe" ]; then
                _bhtest_bin=$BUILD_DIR/bin/$_runner.exe
            else
                bh_error "test runner '$_runner' not found under \"$BUILD_DIR/bin\""
            fi
            bh_run_tests "$_bhtest_bin"
            cd "$BUILD_DIR"
        else
            _step=$((_step + 1))
            printf '(no tests/test.pro found -- nothing to run)\n' \
                | tee "$LOGS/$(printf '%02d-%s.log' "$_step" tests-skipped)"
        fi
    fi

    # Launch (--run).
    bh_run_target
}


# ---- one-stop driver for subdirs (workspace) consumers ----------------

# bh_template_subdirs <top-pro> [argv...]
#
# All-in-one driver for a `TEMPLATE = subdirs` Qt project (one .pro
# orchestrating many SUBDIRS). XD's own build.sh uses this; so does
# any workspace that recurses through multiple sub-.pros.
#
# Each numbered point below captures a structural assumption a
# `TEMPLATE = subdirs` project violates if it tries to drive
# `bh_template_app` instead, and how this driver handles it:
#
#   1. XD_DIR may point at the source root (when XD is *being*
#      built, BH_ROOT == XD_DIR). The check below still requires it
#      to be set so non-self-builders error out cleanly.
#
#   2. BUILD_DIR has NO -mode suffix (subdirs trees typically run
#      debug_and_release + build_all, dropping both flavours into
#      one tree). A project that genuinely wants per-mode dirs can
#      pre-set $BH_BUILD_DIR_SUFFIX (e.g. "-${BH_MODE}").
#
#   3. qmake is NOT given CONFIG+=$BH_MODE -- the subdir .pro files
#      decide mode themselves (debug_and_release, build_all, etc.).
#      bh_make's race-aware release-all/debug-all split serialises
#      the per-config link/mv race.
#
#   4. Plugin search path is adjusted AFTER make (postponed). For
#      projects that BUILD their own plugins (XD itself), pre-make
#      setup would point at an empty/partial tree; post-make works
#      for both self-builders and external consumers. Skip entirely
#      with BH_SKIP_PLUGIN_STAGE=1.
#
#   5. bh_qmake_prepare runs as normal: if the qmake binary is
#      absent at $XD_DIR/bin/qmake, it self-bootstraps via
#      $XD_DIR/qmake/build.sh -- works for XD-as-self-builder.
#
#   6. License gate is the shared bh_license_gate (identical
#      interactive prompt as bh_template_app).
#
#   7. No top-level TARGET resolution -- subdirs has none. --run
#      requires a subdir path and resolves TARGET from that subdir's
#      .pro instead.
#
#   8. --test requires the consumer to have set $BH_TEST_ROOT to its
#      test subdirs .pro. Builds in `$BUILD_DIR/tests-build/` and runs
#      via `make -k check`. Skip with a notice when BH_TEST_ROOT is
#      unset.
#
#   9. --run REQUIRES a relative subdir path
#      (`./build.sh --run path/to/subdir`). The path is taken from
#      $BH_RUN_TARGET_PATH (bh_parse_args consumes it when
#      BH_TEMPLATE=subdirs). We .pro-eval that subdir to find its
#      TARGET, then exec the binary out of $BUILD_DIR/<path>/.
bh_template_subdirs() {
    _pro=$1; shift
    BH_TEMPLATE=subdirs
    bh_parse_args "$@"
    bh_strict_mode
    bh_default_jobs
    bh_default_qmakespec
    bh_macos_developer_dir

    # Phase total for the "(N/M)" tail of the progress line. Each
    # bh_make_step contributes two visible phases (computing +
    # compiling); bh_make's race-aware release/debug split adds
    # two more via its own +=2 bump. Seed assuming a single,
    # non-split bh_make.
    # Base: qmake + make-computing + make-compiling = 3. Resolving
    # and Staging are *conditional* in this template (they only
    # run inside the --test / --run blocks) so they get folded
    # into those flag bumps below, not the base. --wipe is uncounted
    # -- see also docs of: bh_wipe, for why Deleting is uncounted.
    BH_STEPS_TOTAL=3
    [ "${BH_CLEAN:-0}"      -gt 0 ] && BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 1))
    # `--test` runs a *dynamic* sweep -- N matched test projects, each
    # built and run in its own subshell -- so its steps can't share this
    # linear (N/M) counter (subshell `_step` increments don't flow back,
    # and N isn't known up front). The --test block clears BH_STEPS_TOTAL
    # and shows those steps label-only, so nothing is added here for it.
    # `--run` adds: Resolving (bh_target_resolve) = 1; Staging is
    # only counted here when --test didn't already cover it (its
    # `[ -n QT_PLUGINS_STAGE ] ||` short-circuit skips the call).
    [ "${BH_RUN_TARGET:-0}" -gt 0 ] && BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 1))
    if [ "${BH_RUN_TARGET:-0}" -gt 0 ] && [ "${BH_RUN_TESTS:-0}" -eq 0 ]; then
        BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 1))                    # Staging in --run block
    fi

    [ -n "${XD_DIR:-}" ] || bh_error "XD_DIR not set -- resolve the XD framework first (or set XD_DIR=\$ROOT when building XD itself)"

    : "${BUILD_DIR:=$(dirname "$BH_ROOT")/build/$(basename "$BH_ROOT")${BH_BUILD_DIR_SUFFIX:-}}"

    echo "Build directory:"
    echo "  $BUILD_DIR"
    echo
    bh_license_gate

    if [ "${BH_WIPE:-0}" -gt 0 ]; then
        bh_wipe
        [ "${BH_NO_BUILD:-0}" -gt 0 ] && return 0
    fi

    mkdir -p "$BUILD_DIR"
    bh_logs_init "$BUILD_DIR/logs"

    [ "${BH_CLEAN:-0}" -gt 0 ] && bh_clean
    # Plain --no-build (and --only-clean) stop here: clean/wipe only.
    # Exceptions where there's still work to do:
    #   * `--qmake --no-build` regenerates Makefiles (qmake runs below).
    #   * `--no-build --test` / `--no-build --run` mean "the build is
    #     already done -- skip building, but still run the post-build
    #     action". A build that isn't really done then surfaces as a
    #     test/run failure rather than being silently rebuilt.
    bh_can_exit pre-build && return 0

    bh_qmake_prepare

    cd "$BUILD_DIR"
    if [ "${BH_NO_BUILD:-0}" -eq 0 ]; then
        # Normal build: (re)generate Makefiles, then compile.
        # shellcheck disable=SC2086  # intentional word-split of forwarded args.
        bh_run_qmake qmake "$_pro" $BH_REMAINING_ARGS
        bh_make "$JOBS"
    elif [ "${BH_FORCE_QMAKE:-0}" -gt 0 ]; then
        # `--qmake --no-build`: regenerate Makefiles only, no compile.
        # shellcheck disable=SC2086  # intentional word-split of forwarded args.
        bh_run_qmake qmake "$_pro" $BH_REMAINING_ARGS
        # With no post-build action requested, stop after qmake.
        bh_can_exit post-qmake && return 0
    fi
    # else: --no-build with --test/--run and no --qmake -- skip the main
    # build entirely and fall through to the post-build action below.

    # Tests via BH_TEST_ROOT. It holds one project (the consumer's
    # default subdirs test root, or a single `.pro`/literal path), or --
    # when a glob matched -- a sorted, newline-separated list to vet and run
    # (Jest-style: every real match runs). bh_run_one_test builds and runs
    # each in its own build/log subdir and tallies the outcome; a non-zero
    # exit is reported at the end if any test project failed.
    if [ "${BH_RUN_TESTS:-0}" -gt 0 ]; then
        if [ -z "${BH_TEST_ROOT:-}" ]; then
            echo "  (no BH_TEST_ROOT set; skipping --test)" 1>&2
        else
            # The test sweep is dynamic and runs each test in its own
            # subshell, so the linear (N/M) counter doesn't apply here --
            # clear it so the staging + per-test steps render label-only
            # (and don't all collide on the main build's last number).
            BH_STEPS_TOTAL=
            # Postponed plugin staging: only the --test / --run paths
            # consume QT_PLUGINS_STAGE, so we wait until we know we need
            # it. Done once, up front, since every test reuses it.
            bh_run_silent_fg 'Staging Qt plugins for runtime search path' \
                bh_searchpath_plugins
            export DYLD_LIBRARY_PATH=$XD_DIR/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}
            export LD_LIBRARY_PATH=$XD_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
            if [ -n "${QT_PLUGINS_STAGE:-}" ]; then
                export QT_PLUGIN_PATH=$QT_PLUGINS_STAGE${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}
                export QT_QPA_PLATFORM_PLUGIN_PATH=$QT_PLUGINS_STAGE/platforms
            fi

            _bh_tests_total=0
            _bh_tests_failed=0
            if [ "${BH_TEST_GLOB:-0}" -gt 0 ]; then
                # Glob: vet candidates for real-ness in the background
                # while running the ones already vetted, so the checks of
                # the many remaining candidates overlap the current
                # build/run and the next real test is usually already
                # waiting. A plain file queue carries vetted projects from
                # the producer to this consumer in found order -- not a
                # FIFO, since mkfifo named pipes are unreliable under MSYS
                # / Git Bash. The producer appends each real project then
                # writes a done marker; the consumer reads the next unseen
                # line as it appears.
                _bh_reals=0
                _bh_q=$BUILD_DIR/.bh-test-queue
                _bh_done=$BUILD_DIR/.bh-test-done
                rm -f "$_bh_done"
                # Create the queue empty up front so the consumer's `sed`
                # always reads an existing file -- `sed` on a not-yet-
                # created file exits non-zero, which under -e would abort
                # the whole run before the producer's first append.
                : > "$_bh_q"
                # Producer: walk candidates in their fixed order, append
                # only the real test projects, then mark completion. (A
                # failed bh_is_testcase is the left side of &&, so -e
                # leaves it alone. Each `printf` is one short append, so it
                # lands atomically -- the consumer never sees a half line.)
                (
                    IFS='
'
                    for _bhp_c in $BH_TEST_ROOT; do
                        bh_is_testcase "$_bhp_c" && printf '%s\n' "$_bhp_c" >> "$_bh_q"
                    done
                    : > "$_bh_done"
                ) &
                _bh_prod=$!
                # Consumer: run each vetted project as its line appears.
                # While waiting for the producer to surface the next test
                # (before the first one, and in the gaps between), animate
                # a "Searching for test-pattern" spinner; settle it the
                # moment a test is found so that test's output -- including
                # the full -v / -vv log up to the testcase exit -- prints
                # clean below it, then the spinner resumes for the next.
                _bh_line=0
                _bh_searching=0
                while :; do
                    _bh_pro=$(sed -n "$((_bh_line + 1))p" "$_bh_q" 2>/dev/null)
                    if [ -z "$_bh_pro" ]; then
                        # Nothing new. The producer writes every line
                        # before the done marker, so once that exists with
                        # no line pending the queue is fully drained.
                        [ -f "$_bh_done" ] && break
                        if bh_progress_active; then
                            if [ "$_bh_searching" -eq 0 ]; then
                                _bh_searching=1
                                bh_progress_setType spinner
                                bh_progress_setMax 0
                                bh_progress_setDescription \
                                    "Searching for test-pattern: $BH_TEST_PATTERN"
                            fi
                            BH_PROGRESS_VALUE=$((BH_PROGRESS_VALUE + 1))
                            bh_progress_render
                        fi
                        sleep "$BH_SPIN_INTERVAL"
                        continue
                    fi
                    # Found one: settle the spinner before the test runs so
                    # its log starts on a clean row, undisturbed.
                    if [ "$_bh_searching" -ne 0 ]; then
                        bh_progress_hide
                        _bh_searching=0
                    fi
                    _bh_line=$((_bh_line + 1))
                    _bh_reals=$((_bh_reals + 1))
                    bh_run_one_test "$_bh_pro"
                done
                # Clear a still-armed spinner (e.g. zero matches reached
                # the done marker before any test was found).
                [ "$_bh_searching" -ne 0 ] && bh_progress_hide
                wait "$_bh_prod" 2>/dev/null || true
                rm -f "$_bh_q" "$_bh_done"
                [ "$_bh_reals" -gt 0 ] || \
                    bh_error "--test: no test project (CONFIG += testcase) matched the glob"
            else
                # Literal `.pro` or the default subdirs root: trusted
                # entr(y/ies), run as-is without real-ness vetting.
                _bh_oldifs=$IFS
                IFS='
'
                for _bh_pro in $BH_TEST_ROOT; do
                    IFS=$_bh_oldifs
                    bh_run_one_test "$_bh_pro"
                    IFS='
'
                done
                IFS=$_bh_oldifs
            fi
            cd "$BUILD_DIR"
            if [ "$_bh_tests_failed" -gt 0 ]; then
                bh_error "tests: $_bh_tests_failed of $_bh_tests_total test project(s) failed"
            fi
        fi
    fi

    # --run for a specific subdir.
    if [ "${BH_RUN_TARGET:-0}" -gt 0 ]; then
        # If --test already staged plugins above, QT_PLUGINS_STAGE
        # is set and bh_searchpath_plugins is a no-op-ish refresh.
        [ -n "${QT_PLUGINS_STAGE:-}" ] || \
            bh_run_silent_fg 'Staging Qt plugins for runtime search path' \
                bh_searchpath_plugins
        [ -n "${BH_RUN_TARGET_PATH:-}" ] || \
            bh_error "--run under BH_TEMPLATE=subdirs needs a subdir path"
        _bhsub_src=$BH_ROOT/$BH_RUN_TARGET_PATH
        _bhsub_build=$BUILD_DIR/$BH_RUN_TARGET_PATH
        [ -d "$_bhsub_build" ] || bh_error "subdir build not found: $_bhsub_build"
        # Resolve the .pro: prefer Qt's foo/foo.pro convention, fall
        # back to whatever single .pro lives in the subdir.
        _bhsub_base=$(basename "$BH_RUN_TARGET_PATH")
        if [ -f "$_bhsub_src/$_bhsub_base.pro" ]; then
            _bhsub_pro=$_bhsub_src/$_bhsub_base.pro
        else
            # `|| true`: `head -1` closing the pipe SIGPIPEs `ls` when the
            # subdir holds more than one .pro; pipefail would surface 141
            # and abort. Same guard as the freshness probe in bh_run_qmake.
            _bhsub_pro=$(ls "$_bhsub_src"/*.pro 2>/dev/null | head -1 || true)
            [ -n "$_bhsub_pro" ] || bh_error "no .pro file found under $_bhsub_src"
        fi
        bh_target_resolve "$_bhsub_pro"
        if [ -x "$_bhsub_build/$BH_TARGET_NAME.app/Contents/MacOS/$BH_TARGET_NAME" ]; then
            _bin=$_bhsub_build/$BH_TARGET_NAME.app/Contents/MacOS/$BH_TARGET_NAME
        elif [ -x "$_bhsub_build/$BH_TARGET_NAME" ]; then
            _bin=$_bhsub_build/$BH_TARGET_NAME
        elif [ -x "$_bhsub_build/$BH_TARGET_NAME.exe" ]; then
            _bin=$_bhsub_build/$BH_TARGET_NAME.exe
        else
            bh_error "no $BH_TARGET_NAME executable found under $_bhsub_build"
        fi
        echo "Launching $_bin"
        DYLD_LIBRARY_PATH=$XD_DIR/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH} \
        LD_LIBRARY_PATH=$XD_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} \
        QT_PLUGIN_PATH=${QT_PLUGINS_STAGE:-}${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH} \
        QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLUGINS_STAGE:-}/platforms \
        "$_bin"
    fi
}


# bh_template_makefile <drive-fn> <binary-path> [args...]
#
# Template for projects driven by a hand-rolled Makefile (no .pro, no
# qmake step). The caller supplies:
#   <drive-fn>     name of a shell function that runs make for one arch.
#                  It runs with cwd = $BH_ARCH_BUILD_DIR and may read
#                  $BH_CURRENT_ARCH / $BH_MODE / $JOBS to shape its
#                  flags. Typical body calls `bh_make_step`.
#   <binary-path>  absolute path of the binary the makefile writes
#                  (used to copy per-arch outputs aside and lipo them
#                  together on multi-arch macOS builds).
#
# Arch matrix is governed by $BH_ARCHS on macOS:
#   unset / empty       -> host arch only (no lipo)
#   "x86_64"            -> x86_64 only (no lipo)
#   "x86_64 arm64"      -> per-arch builds + `lipo -create` to one fat binary
# Non-macOS hosts always build the host arch once; BH_ARCHS is ignored.
bh_template_makefile() {
    _bhmkf_drive=$1
    _bhmkf_binary=$2
    shift 2
    BH_TEMPLATE=makefile
    bh_parse_args "$@"
    bh_strict_mode
    bh_default_jobs
    bh_default_qmakespec
    bh_macos_developer_dir

    case "$(uname -s)" in
        Darwin) BH_ARCH_LIST=${BH_ARCHS:-$(uname -m)} ;;
        *)      BH_ARCH_LIST=$(uname -m) ;;
    esac
    # shellcheck disable=SC2086
    set -- $BH_ARCH_LIST
    BH_ARCH_COUNT=$#

    BH_STEPS_TOTAL=$((BH_ARCH_COUNT * 2))            # computing + compiling per arch
    [ "$BH_ARCH_COUNT" -gt 1 ] && BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 1))  # lipo
    [ "${BH_CLEAN:-0}" -gt 0 ] && BH_STEPS_TOTAL=$((BH_STEPS_TOTAL + 1))

    [ -n "${BH_ROOT:-}" ] || bh_error "BH_ROOT not set -- caller must set it to the source root"
    : "${BUILD_DIR:=$(dirname "$BH_ROOT")/build/$(basename "$BH_ROOT")-$BH_MODE${BH_BUILD_DIR_SUFFIX:-}}"

    echo "Build directory:"
    echo "  $BUILD_DIR"
    echo
    bh_license_gate

    if [ "${BH_WIPE:-0}" -gt 0 ]; then
        bh_wipe
        [ "${BH_NO_BUILD:-0}" -gt 0 ] && return 0
    fi
    mkdir -p "$BUILD_DIR"
    bh_logs_init "$BUILD_DIR/logs"
    [ "${BH_CLEAN:-0}" -gt 0 ] && bh_clean
    [ "${BH_NO_BUILD:-0}" -gt 0 ] && return 0

    if [ "$BH_ARCH_COUNT" -le 1 ]; then
        BH_CURRENT_ARCH=$BH_ARCH_LIST
        BH_ARCH_BUILD_DIR=$BUILD_DIR
        cd "$BH_ARCH_BUILD_DIR"
        "$_bhmkf_drive"
    else
        _bhmkf_parts=
        for _bhmkf_a in $BH_ARCH_LIST; do
            BH_CURRENT_ARCH=$_bhmkf_a
            BH_ARCH_BUILD_DIR=$BUILD_DIR/$_bhmkf_a
            mkdir -p "$BH_ARCH_BUILD_DIR"
            cd "$BH_ARCH_BUILD_DIR"
            "$_bhmkf_drive"
            # Copy this arch's binary aside before the next arch
            # overwrites it at the makefile's hard-coded output path.
            _bhmkf_part=$BUILD_DIR/parts/$_bhmkf_a/$(basename "$_bhmkf_binary")
            mkdir -p "$(dirname "$_bhmkf_part")"
            cp "$_bhmkf_binary" "$_bhmkf_part"
            _bhmkf_parts="$_bhmkf_parts $_bhmkf_part"
        done
        # shellcheck disable=SC2086  # intentional word-split of part paths.
        bh_run lipo lipo -create $_bhmkf_parts -output "$_bhmkf_binary"
    fi
}

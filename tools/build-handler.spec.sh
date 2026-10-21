#!/bin/sh

# License: Apache 2.0 without attribution need.

# Self-test for `build-handler.sh`. Sources the script and exercises
# every public-facing function whose behaviour doesn't depend on
# qmake/make/sudo/Xcode being usable -- so this file can run on any
# POSIX shell host. Stateful methods (argument parsing, progress
# bookkeeping, defaults) are verified by-value; rendering methods
# (which gate on `[ -t 2 ]`) are checked for code path coverage. The
# last section builds a small project end to end through every build
# mode; it needs XD's qmake, make, and a C++ compiler, and is skipped
# where one is missing.
#
# Run: `sh build-handler.spec.sh`
# Exit: number of failed specs (0 = all pass).


# ---- source target under test + the shared QtTestLib-styled helpers --
#
# All assertion plumbing (test_begin / test_section / expect /
# expect_int_gt / expect_returns / test_end) lives in
# `tools/test-helpers.sh` -- this file is just the call list. Adding
# a new spec means adding `test_section '...'` then a chain of
# `expect ...`; nothing test-framework-shaped lives here anymore.

_spec_dir=${0%[/\\]*}
[ "$_spec_dir" = "$0" ] && _spec_dir=.
# Dummy paths so `bh_error`'s diagnostic format has something to
# interpolate -- no command in this file actually runs `bh_error`.
BH_ROOT=$_spec_dir
BH_SCRIPT_NAME=build-handler.spec.sh
# The `sh -c` runs below source `build-handler.sh` too, with no other way to
# find its `shell-libs` folder (see `bh_tools_dir`).
BH_TOOLS_DIR=$_spec_dir
export BH_TOOLS_DIR
# The builds below end with no note of their build actions, which the note
# specs turn on again (see `bh_progress_actions_note`).
BH_LOG_NOTES=0
export BH_LOG_NOTES
. "$_spec_dir/build-handler.sh"
# `build-handler.sh` loads the helpers of a Windows host only on such a host;
# the specs load them on any host.
. "$_spec_dir/shell-libs/build-handler-win.sh"
. "$_spec_dir/test-helpers.sh"

test_begin 'build-handler.spec'


# ---- specs: bh_parse_args ---------------------------------------------

test_section 'bh_parse_args -- defaults'

unset BH_MODE BH_VERBOSE BH_LOG_LEVEL BH_HEADLESS BH_RUN_TESTS BH_RUN_TARGET
unset BH_NO_PROGRESS BH_CLEAN BH_WIPE BH_NO_BUILD BH_FORCE_QMAKE BH_PREFER_PATH
bh_parse_args
expect 'BH_MODE default is debug'           debug "$BH_MODE"
expect 'BH_LOG_LEVEL default is 1 (normal)' 1     "$BH_LOG_LEVEL"
expect 'BH_HEADLESS default is 0'           0     "$BH_HEADLESS"
expect 'BH_RUN_TESTS default is 0'          0     "$BH_RUN_TESTS"
expect 'BH_RUN_TARGET default is 0'         0     "$BH_RUN_TARGET"
expect 'BH_NO_PROGRESS default is 0'        0     "$BH_NO_PROGRESS"
expect 'BH_CLEAN default is 0'              0     "$BH_CLEAN"
expect 'BH_WIPE default is 0'               0     "$BH_WIPE"
expect 'BH_NO_BUILD default is 0'           0     "$BH_NO_BUILD"
expect 'BH_FORCE_QMAKE default is 0'        0     "$BH_FORCE_QMAKE"
expect 'BH_IGNORE_PRI default is 0'         0     "$BH_IGNORE_PRI"
expect 'BH_PREFER_PATH default is 0'        0     "$BH_PREFER_PATH"

test_section 'bh_parse_args -- individual flags'

unset BH_MODE BH_WIPE BH_FORCE_QMAKE BH_CLEAN BH_HEADLESS BH_VERBOSE
unset BH_LOG_LEVEL BH_RUN_TESTS BH_RUN_TARGET BH_NO_PROGRESS BH_NO_BUILD
bh_parse_args --verbose
expect '--verbose sets BH_LOG_LEVEL=2'      2 "$BH_LOG_LEVEL"

unset BH_LOG_LEVEL
bh_parse_args -v
expect '-v sets BH_LOG_LEVEL=2'             2 "$BH_LOG_LEVEL"

unset BH_LOG_LEVEL BH_VERBOSE
BH_VERBOSE=1
bh_parse_args
expect 'an old truthy BH_VERBOSE maps to BH_LOG_LEVEL=2' 2 "$BH_LOG_LEVEL"
unset BH_VERBOSE

unset BH_RUN_TESTS
bh_parse_args --test
expect '--test sets BH_RUN_TESTS=1'         1 "$BH_RUN_TESTS"

unset BH_RUN_TESTS
bh_parse_args --tests
expect '--tests (plural) sets BH_RUN_TESTS=1' 1 "$BH_RUN_TESTS"

unset BH_MODE
bh_parse_args --release
expect '--release sets BH_MODE=release'     release "$BH_MODE"

unset BH_HEADLESS
bh_parse_args --headless
expect '--headless sets BH_HEADLESS=1'      1 "$BH_HEADLESS"

unset BH_CLEAN
bh_parse_args --clean
expect '--clean sets BH_CLEAN=1'            1 "$BH_CLEAN"

unset BH_WIPE
bh_parse_args --wipe
expect '--wipe sets BH_WIPE=1'              1 "$BH_WIPE"

unset BH_NO_BUILD
bh_parse_args --no-build
expect '--no-build sets BH_NO_BUILD=1'      1 "$BH_NO_BUILD"

unset BH_CLEAN BH_NO_BUILD
bh_parse_args --only-clean
expect '--only-clean sets BH_CLEAN=1'       1 "$BH_CLEAN"
expect '--only-clean sets BH_NO_BUILD=1'    1 "$BH_NO_BUILD"

unset BH_FORCE_QMAKE
bh_parse_args --qmake
expect '--qmake sets BH_FORCE_QMAKE=1'      1 "$BH_FORCE_QMAKE"

unset BH_FORCE_QMAKE
bh_parse_args --refresh
expect '--refresh sets BH_FORCE_QMAKE=1'    1 "$BH_FORCE_QMAKE"

unset BH_IGNORE_PRI
bh_parse_args --ignore-pri
expect '--ignore-pri sets BH_IGNORE_PRI=1'  1 "$BH_IGNORE_PRI"

unset BH_NO_PROGRESS
bh_parse_args --no-progress
expect '--no-progress sets BH_NO_PROGRESS=1' 1 "$BH_NO_PROGRESS"

test_section 'bh_parse_args -- preserves pre-set values (env-var path)'

# A caller-set BH_HEADLESS=1 should survive bh_parse_args even when
# `--headless` is *not* on the cmdline -- the `:= 0` defaults only
# apply to unset variables.
unset BH_HEADLESS
BH_HEADLESS=1
bh_parse_args
expect 'env BH_HEADLESS=1 survives bh_parse_args' 1 "$BH_HEADLESS"

unset BH_MODE
BH_MODE=release
bh_parse_args
expect 'env BH_MODE=release survives bh_parse_args' release "$BH_MODE"

# A given mode builds a subdirs tree in that one variant; without one, the
# tree's own configuration decides, and an app takes CONFIG+=$BH_MODE.
unset BH_MODE BH_MODE_GIVEN BH_DEBUG_INFO BH_SAVE_SPACE BH_MEMSAFE
BH_TEMPLATE=subdirs
bh_parse_args --debug -- CONFIG+=x
expect '--debug: a subdirs tree builds the debug variant only' \
    'CONFIG+=no_debug_and_release CONFIG-=debug_and_release CONFIG-=build_all CONFIG-=release CONFIG+=debug CONFIG+=x' \
    "$BH_REMAINING_ARGS"
unset BH_MODE BH_MODE_GIVEN BH_DEBUG_INFO
bh_parse_args --release-debug
expect '--release-debug: the release variant, with debug info' \
    'CONFIG+=no_debug_and_release CONFIG-=debug_and_release CONFIG-=build_all CONFIG-=debug CONFIG+=release CONFIG+=force_debug_info' \
    "$BH_REMAINING_ARGS"
unset BH_MODE BH_MODE_GIVEN BH_DEBUG_INFO
bh_parse_args -- CONFIG+=x
expect 'no mode: a subdirs tree keeps its own variants' 'CONFIG+=x' "$BH_REMAINING_ARGS"
unset BH_TEMPLATE BH_MODE BH_MODE_GIVEN BH_DEBUG_INFO
bh_parse_args --debug -- CONFIG+=x
expect '--debug: an app gets no variant values here' 'CONFIG+=x' "$BH_REMAINING_ARGS"
unset BH_MODE BH_MODE_GIVEN BH_REMAINING_ARGS

test_section 'bh_parse_args -- remaining args'

unset BH_REMAINING_ARGS
bh_parse_args --headless -- CONFIG+=foo bar
expect '`--` stops parsing and remainder hits BH_REMAINING_ARGS' \
    'CONFIG+=foo bar' "$BH_REMAINING_ARGS"

unset BH_REMAINING_ARGS
bh_parse_args --headless CONFIG+=qux
expect 'first non-flag positional also lands in BH_REMAINING_ARGS' \
    'CONFIG+=qux' "$BH_REMAINING_ARGS"


# MARK: specs: bh_find_filc.

test_section 'bh_find_filc -- Fil-C-Light first, else the first working Fil-C'

# Fake toolchains in a sandbox stand in for the installed one and for the
# trees around a checkout at `top/mid/xd`; each fake `clang++` names its own
# toolchain in its Fil-C version banner, hence a case reads which one won.

_bh_spec_sandbox=$(mktemp -d)
mkdir -p "$_bh_spec_sandbox/top/mid/xd"

# Writes a fake Fil-C compiler at <file> that names <tag> in its banner.
_bh_spec_fake_filc() {
    mkdir -p "${1%/*}"
    printf '#!/bin/sh\necho "clang version 20 (Fil-C 0.686) %s"\n' "$2" > "$1"
    chmod +x "$1"
}

# Makes a fake tree at `top/<path>`; a second argument `light` adds the
# marker of a Fil-C-Light toolchain.
_bh_spec_filc_tree() {
    _bh_spec_fake_filc "$_bh_spec_sandbox/top/$1/build/bin/clang++" "$1"
    if [ "${2:-}" = light ]; then
        mkdir -p "$_bh_spec_sandbox/top/$1/build/share"
        : > "$_bh_spec_sandbox/top/$1/build/share/fil-c-light.ini"
    fi
}

# Runs the search for the sandbox checkout, in a subshell, and prints the
# banner tag of the `clang++` it put first on `PATH`, or `none`.
_bh_spec_find_filc() (
    BH_ROOT=$_bh_spec_sandbox/top/mid/xd
    BH_FILC_OPT_DIR=$_bh_spec_sandbox/opt
    XDG_CACHE_HOME=$_bh_spec_sandbox/cache
    _bh_spec_path=$PATH
    if bh_find_filc; then
        clang++ --version | sed 's/.*) //'
    elif [ "$PATH" = "$_bh_spec_path" ]; then
        echo none
    else
        echo "none, yet PATH changed"
    fi
)

_bh_spec_links=$_bh_spec_sandbox/cache/build-handler/filc-links/filcc-clang-20

expect 'no toolchain anywhere -> none, PATH kept' none "$(_bh_spec_find_filc)"

_bh_spec_fake_filc "$_bh_spec_sandbox/opt/bin/filcc-clang-20" opt
expect 'only an upstream installed toolchain -> it, through clang links' \
    opt "$(_bh_spec_find_filc)"
expect 'the clang++ link names the installed compiler' \
    "$_bh_spec_sandbox/opt/bin/filcc-clang-20" \
    "$(readlink "$_bh_spec_links/clang++")"
expect 'the clang link names the installed compiler' \
    "$_bh_spec_sandbox/opt/bin/filcc-clang-20" \
    "$(readlink "$_bh_spec_links/clang")"
_bh_spec_inode=$(ls -id "$_bh_spec_links/clang++")
_bh_spec_find_filc > /dev/null
expect 'a link that already names the compiler is not made again' \
    "$_bh_spec_inode" "$(ls -id "$_bh_spec_links/clang++")"

_bh_spec_filc_tree mid/filc
expect 'an upstream tree loses to the earlier installed one' \
    opt "$(_bh_spec_find_filc)"

_bh_spec_filc_tree filc-light light
expect 'a Fil-C-Light tree one level higher wins over upstream ones' \
    filc-light "$(_bh_spec_find_filc)"

_bh_spec_filc_tree mid/fil-c-light
expect 'an unmarked tree, though named Light, loses to a marked one' \
    filc-light "$(_bh_spec_find_filc)"

_bh_spec_filc_tree fil-c-light light
expect '../../fil-c-light comes before ../../filc-light' \
    fil-c-light "$(_bh_spec_find_filc)"

mkdir -p "$_bh_spec_sandbox/opt/share"
: > "$_bh_spec_sandbox/opt/share/fil-c-light.ini"
expect 'a Fil-C-Light installed toolchain comes first' \
    opt "$(_bh_spec_find_filc)"

rm -rf "$_bh_spec_sandbox/opt"
expect 'a relative BH_ROOT still puts an absolute folder on PATH' \
    "$_bh_spec_sandbox/top/fil-c-light/build/bin/clang++" \
    "$(cd "$_bh_spec_sandbox/top" && BH_ROOT=mid/xd \
        BH_FILC_OPT_DIR=$_bh_spec_sandbox/opt bh_find_filc && command -v clang++)"

rm -rf "$_bh_spec_sandbox"


test_section 'bh_parse_args -- --prefer-path takes a Fil-C clang++ already on PATH'

# A Fil-C-Light tree sits beside the sandbox checkout, while another Fil-C
# `clang++`, or a plain one, comes first on `PATH`; each case prints the
# compiler `--memory-safe` settles on, relative to the sandbox.

_bh_spec_sandbox=$(mktemp -d)
mkdir -p "$_bh_spec_sandbox/top/mid/xd"
_bh_spec_filc_tree fil-c-light light
_bh_spec_fake_filc "$_bh_spec_sandbox/filc-path/clang++" path
mkdir -p "$_bh_spec_sandbox/plain-path"
printf '#!/bin/sh\necho "clang version 20.1.8 (plain)"\n' \
    > "$_bh_spec_sandbox/plain-path/clang++"
chmod +x "$_bh_spec_sandbox/plain-path/clang++"

# Runs `bh_parse_args --memory-safe <arguments>` in a subshell, with the
# folder <first on PATH> of the sandbox ahead of the rest of `PATH`.
_bh_spec_memsafe() (
    _bh_spec_first=$1
    shift
    PATH=$_bh_spec_sandbox/$_bh_spec_first:$PATH
    BH_ROOT=$_bh_spec_sandbox/top/mid/xd
    BH_FILC_OPT_DIR=$_bh_spec_sandbox/opt
    XDG_CACHE_HOME=$_bh_spec_sandbox/cache
    unset BH_PREFER_PATH BH_MEMSAFE
    bh_parse_args --headless --memory-safe "$@" > /dev/null 2>&1
    echo "${BH_MEMSAFE_CXX#"$_bh_spec_sandbox"/}"
)

expect 'without --prefer-path a Fil-C-Light tree beats a Fil-C on PATH' \
    top/fil-c-light/build/bin/clang++ "$(_bh_spec_memsafe filc-path)"
expect '--prefer-path takes the Fil-C on PATH' \
    filc-path/clang++ "$(_bh_spec_memsafe filc-path --prefer-path)"
expect '--prefer-path passes over a clang++ on PATH that is no Fil-C one' \
    top/fil-c-light/build/bin/clang++ "$(_bh_spec_memsafe plain-path --prefer-path)"

rm -rf "$_bh_spec_sandbox"


# MARK: specs: bh_make_find and bh_make_run.

test_section 'bh_make_run -- the make tool of the spec, with arguments it takes'

# Fake `make`, `nmake` and `jom` print their name, the folder they run in and
# their arguments; each case runs in a subshell with no make tool picked yet.
# The fake `jom` joins in last, once the cases of nmake alone are done.

_bh_spec_sandbox=$(mktemp -d)
mkdir -p "$_bh_spec_sandbox/bin" "$_bh_spec_sandbox/sub"
for _bh_spec_tool in make nmake jom; do
    printf '#!/bin/sh\necho "%s ${PWD##*/} $*"\n' "$_bh_spec_tool" \
        > "$_bh_spec_sandbox/bin/$_bh_spec_tool"
    chmod +x "$_bh_spec_sandbox/bin/$_bh_spec_tool"
done
mv "$_bh_spec_sandbox/bin/jom" "$_bh_spec_sandbox/jom"

# Runs `bh_make_run` with <arguments> under the target spec <spec>, from the
# sandbox, with the fake tools first on `PATH`.
_bh_spec_make() (
    unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC
    QMAKESPEC=$1
    shift
    PATH=$_bh_spec_sandbox/bin:$PATH
    cd "$_bh_spec_sandbox" && bh_make_run "$@"
)

expect 'a GNU make spec runs make with the arguments as given' \
    "make ${_bh_spec_sandbox##*/} -j4 -k qmake_all" \
    "$(_bh_spec_make linux-g++ -j4 -k qmake_all)"
expect '-C <dir> runs the tool in <dir>' \
    'make sub -k clean' "$(_bh_spec_make linux-g++ -C sub -k clean)"
expect 'an MSVC spec runs nmake, without -j<n>' \
    "nmake ${_bh_spec_sandbox##*/} -nologo -f Makefile.Release release-all" \
    "$(_bh_spec_make win32-msvc2017 -j4 -f Makefile.Release release-all)"
expect 'nmake also drops -j <n>, and keeps a number that is no job count' \
    "nmake ${_bh_spec_sandbox##*/} -nologo -n 7" \
    "$(_bh_spec_make win32-msvc2015 -j 2 -n 7)"
expect 'nmake drops a trailing -j' \
    "nmake ${_bh_spec_sandbox##*/} -nologo 8 -k" \
    "$(_bh_spec_make win32-msvc2015 8 -k -j)"
expect '-C <dir> runs nmake in <dir>' \
    'nmake sub -nologo mocables' "$(_bh_spec_make win32-msvc2017 -C sub mocables)"
expect 'the cross target spec decides over the host one' \
    "make ${_bh_spec_sandbox##*/} all" \
    "$(BH_CROSS_SPEC=win32-g++; QMAKESPEC=win32-msvc2017; unset BH_MAKE BH_MAKE_TYPE
       PATH=$_bh_spec_sandbox/bin:$PATH; cd "$_bh_spec_sandbox" && bh_make_run all)"
expect 'the tool found is kept by absolute path, with its type' \
    "$_bh_spec_sandbox/bin/nmake nmake" \
    "$(unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC; QMAKESPEC=win32-msvc2017
       PATH=$_bh_spec_sandbox/bin:$PATH; bh_make_find; echo "$BH_MAKE $BH_MAKE_TYPE")"
expect 'a GNU make spec keeps make by absolute path, as type gnu' \
    "$_bh_spec_sandbox/bin/make gnu" \
    "$(unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC; QMAKESPEC=linux-g++
       PATH=$_bh_spec_sandbox/bin:$PATH; bh_make_find; echo "$BH_MAKE $BH_MAKE_TYPE")"
expect 'a preset BH_MAKE is kept, its type taken from its name' \
    "nmake ${_bh_spec_sandbox##*/} -nologo all" \
    "$(BH_MAKE=nmake QMAKESPEC=linux-g++; unset BH_MAKE_TYPE
       PATH=$_bh_spec_sandbox/bin:$PATH; cd "$_bh_spec_sandbox" && bh_make_run all)"
expect 'a preset BH_MAKE_TYPE decides how the options are passed' \
    "nmake ${_bh_spec_sandbox##*/} -j4 all" \
    "$(BH_MAKE=nmake BH_MAKE_TYPE=gnu; PATH=$_bh_spec_sandbox/bin:$PATH
       cd "$_bh_spec_sandbox" && bh_make_run -j4 all)"
expect_returns 'an unknown BH_MAKE_TYPE is an error' \
    1 sh -c 'BH_MAKE=make BH_MAKE_TYPE=bsd
             . "'"$_spec_dir"'/build-handler.sh"; bh_make_run all'

mv "$_bh_spec_sandbox/jom" "$_bh_spec_sandbox/bin/jom"
expect 'an MSVC spec takes jom over nmake, by absolute path' \
    "$_bh_spec_sandbox/bin/jom jom" \
    "$(unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC; QMAKESPEC=win32-msvc2017
       PATH=$_bh_spec_sandbox/bin:$PATH; bh_make_find; echo "$BH_MAKE $BH_MAKE_TYPE")"
expect 'jom gets -j<n> as -j <n>' \
    "jom ${_bh_spec_sandbox##*/} -nologo -j 4 -f Makefile.Release release-all" \
    "$(_bh_spec_make win32-msvc2017 -j4 -f Makefile.Release release-all)"
expect 'jom keeps -j <n>, and drops a bare -j' \
    "jom ${_bh_spec_sandbox##*/} -nologo -j 2 -k all" \
    "$(_bh_spec_make win32-msvc2017 -j 2 -k -j all)"
expect '-C <dir> runs jom in <dir>' \
    'jom sub -nologo -k clean' "$(_bh_spec_make win32-msvc2015 -C sub -k clean)"
expect 'a preset BH_MAKE named jom is of type jom' \
    "jom ${_bh_spec_sandbox##*/} -nologo -j 3 all" \
    "$(BH_MAKE=jom; unset BH_MAKE_TYPE; PATH=$_bh_spec_sandbox/bin:$PATH
       cd "$_bh_spec_sandbox" && bh_make_run -j3 all)"
expect 'bh_make_find_gnu picks GNU make even for an MSVC spec' \
    "make ${_bh_spec_sandbox##*/} -j2 -f Makefile.unix" \
    "$(unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC; QMAKESPEC=win32-msvc2017
       PATH=$_bh_spec_sandbox/bin:$PATH; cd "$_bh_spec_sandbox" && bh_make_find_gnu \
       && bh_make_run -j2 -f Makefile.unix)"
mv "$_bh_spec_sandbox/bin/make" "$_bh_spec_sandbox/bin/mingw32-make"
expect 'GNU make named mingw32-make serves where make is missing' \
    "$_bh_spec_sandbox/bin/mingw32-make gnu" \
    "$(unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC; QMAKESPEC=win32-g++
       PATH=$_bh_spec_sandbox/bin; bh_make_find; echo "$BH_MAKE $BH_MAKE_TYPE")"
mv "$_bh_spec_sandbox/bin/mingw32-make" "$_bh_spec_sandbox/bin/make"
rm "$_bh_spec_sandbox/bin/nmake"
expect 'jom alone serves an MSVC spec' \
    "jom ${_bh_spec_sandbox##*/} -nologo all" \
    "$(_bh_spec_make win32-msvc2017 all)"
rm "$_bh_spec_sandbox/bin/jom"
expect_returns 'an MSVC spec with neither jom nor nmake on PATH is an error' \
    1 _bh_spec_make win32-msvc2017 all

# Writes a fake `jom` that reports <version>, ended the way a Windows program
# ends a line (`\r\n`), for `-VERSION`, as the real one does.
_bh_spec_versioned_jom() {
    printf '#!/bin/sh\nif [ "$1" = -VERSION ]; then\n    printf "jom version %s\\r\\n"\n    exit 0\nfi\necho "jom ${PWD##*/} $*"\n' \
        "$1" > "$_bh_spec_sandbox/bin/jom"
    chmod +x "$_bh_spec_sandbox/bin/jom"
}

# Runs `bh_make_find` under an MSVC spec, then prints the tool it picked and,
# below it, the warnings it wrote.
_bh_spec_find_msvc() (
    unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC
    QMAKESPEC=win32-msvc2017
    PATH=$_bh_spec_sandbox/bin:$PATH
    bh_make_find 2>"$_bh_spec_sandbox/warnings"
    echo "$BH_MAKE $BH_MAKE_TYPE"
    cat "$_bh_spec_sandbox/warnings"
)

printf '#!/bin/sh\necho "nmake ${PWD##*/} $*"\n' > "$_bh_spec_sandbox/bin/nmake"
chmod +x "$_bh_spec_sandbox/bin/nmake"
_bh_spec_versioned_jom 1.1.3
expect 'a jom newer than 1.0.16 is taken over nmake' \
    "$_bh_spec_sandbox/bin/jom jom" "$(_bh_spec_find_msvc)"
_bh_spec_versioned_jom 1.0.16
expect 'jom 1.0.16 is taken over nmake' \
    "$_bh_spec_sandbox/bin/jom jom" "$(_bh_spec_find_msvc)"
_bh_spec_versioned_jom 1.0.13
expect 'an older jom gives way to nmake, with a warning' \
    "$_bh_spec_sandbox/bin/nmake nmake

WARNING: jom 1.0.13 can hang once a command fails, hence
         nmake.exe runs the Makefiles instead.
         Update jom to 1.0.16 or newer to build in parallel:
         https://download.qt.io/official_releases/jom/" \
    "$(_bh_spec_find_msvc)"
rm "$_bh_spec_sandbox/bin/nmake"
expect 'an older jom still runs when nmake is missing, with a warning' \
    "$_bh_spec_sandbox/bin/jom jom

WARNING: jom 1.0.13 can hang once a command fails, and
         no nmake.exe is on PATH to run instead.
         Update jom to 1.0.16 or newer:
         https://download.qt.io/official_releases/jom/" \
    "$(_bh_spec_find_msvc)"

rm -rf "$_bh_spec_sandbox"

test_section 'bh_make_run -- the folder of cl comes first for MSVC Makefiles'

# A fake `nmake` prints which `link` it finds; a `link` like Git Bash's sits
# earlier on PATH than the folder of the fake `cl` and its `link`.
_bh_spec_sandbox=$(mktemp -d)
mkdir -p "$_bh_spec_sandbox/usr-bin" "$_bh_spec_sandbox/msvc"
for _bh_spec_tool in usr-bin/link msvc/cl msvc/link msvc/nmake; do
    printf '#!/bin/sh\ncommand -v link\n' > "$_bh_spec_sandbox/$_bh_spec_tool"
    chmod +x "$_bh_spec_sandbox/$_bh_spec_tool"
done

expect "nmake finds the link beside cl, not Git Bash's" \
    "$_bh_spec_sandbox/msvc/link" \
    "$(unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC; QMAKESPEC=win32-msvc2010
       PATH=$_bh_spec_sandbox/usr-bin:$_bh_spec_sandbox/msvc:$PATH
       bh_make_run all)"
expect 'the PATH of the script stays as it was' \
    "$_bh_spec_sandbox/usr-bin/link" \
    "$(unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC; QMAKESPEC=win32-msvc2010
       PATH=$_bh_spec_sandbox/usr-bin:$_bh_spec_sandbox/msvc:$PATH
       bh_make_run all >/dev/null; command -v link)"
rm "$_bh_spec_sandbox/msvc/link"
expect 'with no link beside cl, PATH stays as it was for nmake too' \
    "$_bh_spec_sandbox/usr-bin/link" \
    "$(unset BH_MAKE BH_MAKE_TYPE BH_CROSS_SPEC; QMAKESPEC=win32-msvc2010
       PATH=$_bh_spec_sandbox/usr-bin:$_bh_spec_sandbox/msvc:$PATH
       bh_make_run all)"
rm -rf "$_bh_spec_sandbox"

test_section 'bh_path_prepend -- a folder moves first, with no copy left'

expect 'the folder moves first, and its copies in other letter case go' \
    '/d/WinDDK/VC/bin:/usr/bin:/c/x:/c/y' \
    "$(bh_path_prepend /d/WinDDK/VC/bin '/usr/bin:/D/WINDDK/vc/bin:/c/x:/d/winddk/VC/bin:/c/y')"
expect 'a copy with a trailing slash goes too' \
    '/d/vc:/usr/bin' "$(bh_path_prepend /d/vc '/usr/bin:/d/vc/')"
expect 'a folder not on PATH is only added' \
    '/a/b:/c:/d' "$(bh_path_prepend /a/b '/c:/d')"
expect 'empty entries stay' \
    '/a/b:/c::/d' "$(bh_path_prepend /a/b '/c::/a/b:/d')"
expect 'a folder that only starts like another one stays' \
    '/a/b:/a/bc:/a/b/c' "$(bh_path_prepend /a/b '/a/bc:/a/b/c')"
expect 'PATH is used when no path is given' \
    "/a/b:/c:$PATH" "$(PATH=/c:$PATH; bh_path_prepend /a/b)"
expect 'an empty last entry stays' \
    '/a/b:/c:' "$(bh_path_prepend /a/b '/c:/a/b:')"
expect 'an empty path gives the folder alone' \
    /a/b "$(bh_path_prepend /a/b '')"
expect 'a backslash in the folder stays as it is' \
    '/a\tb:/c' "$(bh_path_prepend '/a\tb' /c)"

test_section 'bh_msvc_spec -- the mkspec of the Visual Studio on PATH'

# Fake `cl` that prints the banner of the compiler <version>, as the real one
# does on stderr when it gets no source.
_bh_spec_sandbox=$(mktemp -d)
_bh_spec_fake_cl() {
    printf '#!/bin/sh\necho "Microsoft (R) C/C++ Optimizing Compiler Version %s for x64" 1>&2\n' \
        "$1" > "$_bh_spec_sandbox/cl"
    chmod +x "$_bh_spec_sandbox/cl"
}

# Runs `bh_msvc_spec` with the fake `cl` first on `PATH`, and no
# `VisualStudioVersion`.
_bh_spec_msvc_spec() (
    unset VisualStudioVersion
    PATH=$_bh_spec_sandbox:$PATH
    bh_msvc_spec
)

for _bh_spec_pair in 14.00.50727.762:win32-msvc2005 \
        15.00.30729.01:win32-msvc2008 16.00.40219.01:win32-msvc2010 \
        17.00.61030:win32-msvc2012 18.00.40629:win32-msvc2013 \
        19.00.24215.1:win32-msvc2015 19.16.27045:win32-msvc2017 \
        19.29.30154:win32-msvc2017; do
    _bh_spec_fake_cl "${_bh_spec_pair%%:*}"
    expect "cl ${_bh_spec_pair%%:*} takes ${_bh_spec_pair#*:}" \
        "${_bh_spec_pair#*:}" "$(_bh_spec_msvc_spec)"
done
expect 'VisualStudioVersion 10.0 takes win32-msvc2010, whatever cl says' \
    win32-msvc2010 "$(VisualStudioVersion=10.0 PATH=$_bh_spec_sandbox:$PATH bh_msvc_spec)"
expect 'VisualStudioVersion 11.0 takes win32-msvc2012' \
    win32-msvc2012 "$(VisualStudioVersion=11.0 PATH=$_bh_spec_sandbox:$PATH bh_msvc_spec)"
rm -rf "$_bh_spec_sandbox"

test_section 'bh_makefile_spec_differs -- a Makefile made for other specs'

# Writes a Makefile whose `# Command:` line is the qmake call <command>, the
# way qmake heads the Makefiles it writes.
_bh_spec_sandbox=$(mktemp -d)
_bh_spec_command_makefile() {
    printf '%s\n' '#####' '# Makefile for building: x' \
        "# Command: $1" '#####' 'all:' > "$_bh_spec_sandbox/Makefile"
}

_bh_spec_command_makefile 'D:/xd/bin/qmake.exe -r -spec D:/xd/mkspecs/win32-msvc2015 -xspec D:/xd/mkspecs/win32-msvc2015 -o Makefile D:\xd\XD-mini.pro'
expect_returns 'a Makefile for win32-msvc2015 differs from win32-msvc2010' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=win32-msvc2010
             bh_makefile_spec_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'a Makefile for the current spec does not differ' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=win32-msvc2015
             bh_makefile_spec_differs "'"$_bh_spec_sandbox"'/Makefile"'
_bh_spec_command_makefile 'D:\xd\bin\qmake.exe -spec D:\xd\mkspecs\win32-msvc2010 -o Makefile x.pro'
expect_returns 'a spec path with backslashes is read by its last folder' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=win32-msvc2010
             bh_makefile_spec_differs "'"$_bh_spec_sandbox"'/Makefile"'
_bh_spec_command_makefile '/xd/bin/qmake -r -spec /xd/mkspecs/linux-g++ -xspec /xd/mkspecs/win32-g++ x.pro'
expect_returns 'a Makefile for another cross target differs' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-g++
             BH_CROSS_SPEC=win32-clang
             bh_makefile_spec_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'a Makefile for the current cross target does not differ' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-g++
             BH_CROSS_SPEC=win32-g++
             bh_makefile_spec_differs "'"$_bh_spec_sandbox"'/Makefile"'
_bh_spec_command_makefile '/xd/bin/qmake -o Makefile x.pro'
expect_returns 'a Makefile that names no spec counts as current' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-g++
             bh_makefile_spec_differs "'"$_bh_spec_sandbox"'/Makefile"'
_bh_spec_command_makefile "/x d/bin/qmake -r -spec '/x d/mkspecs/linux-g++' -xspec '/x d/mkspecs/linux-g++' -o Makefile x.pro"
expect_returns 'a spec path in single quotes, holding a space, is current' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-g++
             bh_makefile_spec_differs "'"$_bh_spec_sandbox"'/Makefile"'
_bh_spec_command_makefile 'D:\x d\bin\qmake.exe -spec "D:/x d/mkspecs/win32-msvc2015" -xspec "D:/x d/mkspecs/win32-msvc2015" -o Makefile x.pro'
expect_returns 'a spec path in double quotes, holding a space, is read too' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=win32-msvc2010
             bh_makefile_spec_differs "'"$_bh_spec_sandbox"'/Makefile"'
_bh_spec_command_makefile '/xd/bin/qmake -r -spec /xd/mkspecs/linux-clang -xspec /xd/mkspecs/linux-clang CONFIG+=memory_safe -o Makefile x.pro'
expect_returns 'a memory-safe Makefile is of another toolchain than a plain run' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-clang
             bh_makefile_toolchain_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'but of the same as a memory-safe run' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-clang
             BH_MEMSAFE=filc
             bh_makefile_toolchain_differs "'"$_bh_spec_sandbox"'/Makefile"'
_bh_spec_command_makefile '/xd/bin/qmake -r -spec /xd/mkspecs/linux-clang -xspec /xd/mkspecs/linux-clang -o Makefile x.pro'
expect_returns 'a plain Makefile is of another toolchain than a memory-safe run' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-clang
             BH_MEMSAFE=filc
             bh_makefile_toolchain_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'a Makefile for other specs is of another toolchain too' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-g++
             bh_makefile_toolchain_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'a Makefile naming no variant is of the variant of a run with none' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; BH_REMAINING_ARGS=CONFIG+=x
             bh_makefile_variant_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'but of another variant than a --debug run' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; BH_MODE=debug
             BH_REMAINING_ARGS=$(bh_variant_qmake_args)
             bh_makefile_variant_differs "'"$_bh_spec_sandbox"'/Makefile"'
_bh_spec_command_makefile '/xd/bin/qmake -r -spec /xd/mkspecs/linux-clang -xspec /xd/mkspecs/linux-clang CONFIG+=memory_safe CONFIG+=no_debug_and_release CONFIG-=debug_and_release CONFIG-=build_all CONFIG-=release CONFIG+=debug -o Makefile x.pro'
expect_returns 'a debug-only Makefile is of the variant of a --debug run' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; BH_MODE=debug
             BH_REMAINING_ARGS=$(bh_variant_qmake_args)
             bh_makefile_variant_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'but of another variant than a --release run' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; BH_MODE=release
             BH_REMAINING_ARGS=$(bh_variant_qmake_args)
             bh_makefile_variant_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'and of another build than a memory-safe --release run' \
    0 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-clang
             BH_MEMSAFE=filc BH_MODE=release
             BH_REMAINING_ARGS="CONFIG+=memory_safe $(bh_variant_qmake_args)"
             bh_makefile_build_differs "'"$_bh_spec_sandbox"'/Makefile"'
expect_returns 'not of another build than a memory-safe --debug run' \
    1 sh -c '. "'"$_spec_dir"'/build-handler.sh"; QMAKESPEC=linux-clang
             BH_MEMSAFE=filc BH_MODE=debug
             BH_REMAINING_ARGS="CONFIG+=memory_safe $(bh_variant_qmake_args)"
             bh_makefile_build_differs "'"$_bh_spec_sandbox"'/Makefile"'
rm -rf "$_bh_spec_sandbox"

test_section 'bh_command_option -- the argument after an option'

expect 'a plain argument' /xd/mkspecs/linux-g++ \
    "$(bh_command_option 'qmake -spec /xd/mkspecs/linux-g++ -o Makefile x.pro' -spec)"
expect 'an argument in single quotes, without them' '/x d/mkspecs/linux-g++' \
    "$(bh_command_option "qmake -spec '/x d/mkspecs/linux-g++' x.pro" -spec)"
expect 'an argument in double quotes, without them' 'D:/x d/mkspecs/win32-g++' \
    "$(bh_command_option 'qmake -xspec "D:/x d/mkspecs/win32-g++" x.pro' -xspec)"
expect '-spec does not read the argument of -xspec' '' \
    "$(bh_command_option 'qmake -xspec /xd/mkspecs/win32-g++ x.pro' -spec)"
expect 'nothing when the option is missing' '' \
    "$(bh_command_option 'qmake -o Makefile x.pro' -spec)"

test_section 'bh_template_makefile -- --rebuild builds everything again'

# The drive function only lists what the build folder holds when it runs.
_bh_spec_sandbox=$(mktemp -d)
_bh_spec_makefile_build() {
    ( . "$_spec_dir/build-handler.sh"
      _bh_spec_drive() { ls "$BUILD_DIR" | tr '\n' ' '; }
      unset BH_WIPE BH_CLEAN BH_REBUILD BH_NO_BUILD BH_GET BH_MODE BH_FILTER
      BH_ROOT=$_bh_spec_sandbox/src BUILD_DIR=$_bh_spec_sandbox/bd
      bh_template_makefile _bh_spec_drive "$_bh_spec_sandbox/bin" --headless \
          --no-progress "$@" 2>/dev/null | tail -1 )
}
mkdir -p "$_bh_spec_sandbox/src" "$_bh_spec_sandbox/bd"
: > "$_bh_spec_sandbox/bd/main.o"
expect 'a build keeps the objects of the last one' 'logs main.o ' \
    "$(_bh_spec_makefile_build)"
expect '--rebuild removes them first, keeping the logs' 'logs ' \
    "$(_bh_spec_makefile_build --rebuild)"
: > "$_bh_spec_sandbox/bd/main.o"
expect '--clean removes them too' 'logs ' "$(_bh_spec_makefile_build --clean)"
rm -rf "$_bh_spec_sandbox"

test_section 'build-handler-win.sh -- loaded on a Windows host'

# A stub `uname` first on PATH names a Windows host; bash stands in for the
# shell there, as it names the sourced file in `BASH_SOURCE`.
_bh_spec_sandbox=$(mktemp -d)
printf '#!/bin/sh\necho MINGW64_NT-10.0\n' > "$_bh_spec_sandbox/uname"
chmod +x "$_bh_spec_sandbox/uname"
# _bh_spec_load <code>
# Sources build-handler.sh in bash on the stub Windows host, then runs <code>.
_bh_spec_load() {
    PATH=$_bh_spec_sandbox:$PATH bash -c '. "$1/build-handler.sh"; '"$1" \
        _bh_spec_load "$_spec_dir"
}
expect 'a Windows host adds the on-exit tool check' 1 \
    "$(_bh_spec_load 'printf "%s\n" "$BH_HOOKS_on_exit"' | grep -cx bh_tool_check_on_failure)"
expect 'another host loads nothing of it' 0 \
    "$(bash -c '. "$1/build-handler.sh"; printf "%s\n" "${BH_HOOKS_on_exit:-}"' \
        _bh_spec_load "$_spec_dir" | grep -cx bh_tool_check_on_failure)"
rm -rf "$_bh_spec_sandbox"

test_section 'bh_path_before_windows -- the POSIX folder before the Windows ones'

expect 'before the first Windows folder, others keep their order' \
    '/mingw64/bin:/usr/bin:/c/WINDOWS/system32:/c/x' \
    "$(bh_path_before_windows /usr/bin '/mingw64/bin:/c/WINDOWS/system32:/usr/bin:/c/x')"
expect 'already first, its later copies dropped' '/usr/bin:/c/Windows/system32:' \
    "$(bh_path_before_windows /usr/bin '/usr/bin:/c/Windows/system32::/usr/bin/')"
expect 'missing, before the Windows folder still' '/a:/usr/bin:/c/windows' \
    "$(bh_path_before_windows /usr/bin '/a:/c/windows')"
expect 'no Windows folder' '/a:/b' "$(bh_path_before_windows /usr/bin '/a:/b')"
expect 'a folder only named like one' '/a/windowsx:/b' \
    "$(bh_path_before_windows /usr/bin '/a/windowsx:/b')"

# A sandbox stands in for a System32 folder first on PATH.
_bh_spec_sandbox=$(mktemp -d)
mkdir -p "$_bh_spec_sandbox/Windows/System32"
printf '#!/bin/sh\necho windows "$@"\n' > "$_bh_spec_sandbox/Windows/System32/find"
chmod +x "$_bh_spec_sandbox/Windows/System32/find"

test_section 'bh_tool_check -- external tools missing or Windows ones'

_bh_spec_check() {
    ( BH_EXTERNAL_TOOLS=$1
      PATH=$_bh_spec_sandbox/Windows/System32:$PATH
      bh_tool_check 2>&1 )
}
expect 'nothing when each tool is fine' '' "$(_bh_spec_check sh)"
expect 'a Windows tool' \
    "find: runs $_bh_spec_sandbox/Windows/System32/find, the Windows tool, not the POSIX one." \
    "$(_bh_spec_check find | sed -n 's/^ *//; /^find:/p')"
expect 'a missing tool' 'bh-spec-none: not found.' \
    "$(_bh_spec_check 'sh bh-spec-none' | sed -n 's/^ *//; /^bh-spec-none:/p')"
mkdir -p "$_bh_spec_sandbox/posix" "$_bh_spec_sandbox/other"
printf '#!/bin/sh\n' > "$_bh_spec_sandbox/posix/sort"
cp "$_bh_spec_sandbox/posix/sort" "$_bh_spec_sandbox/other/sort"
chmod +x "$_bh_spec_sandbox/posix/sort" "$_bh_spec_sandbox/other/sort"
_bh_spec_shadow() {
    ( BH_EXTERNAL_TOOLS=sort; PATH=$1:$PATH
      bh_tool_check "$_bh_spec_sandbox/posix" 2>&1 | sed -n 's/^ *//; /^sort:/p' )
}
expect 'another copy that shadows the POSIX one' \
    "sort: runs $_bh_spec_sandbox/other/sort, which shadows the POSIX $_bh_spec_sandbox/posix/sort." \
    "$(_bh_spec_shadow "$_bh_spec_sandbox/other:$_bh_spec_sandbox/posix")"
expect 'the POSIX copy itself' '' "$(_bh_spec_shadow "$_bh_spec_sandbox/posix")"
expect 'a failed run ends with the check' 'bh-spec-none: not found.' \
    "$( ( BH_EXTERNAL_TOOLS=bh-spec-none; trap '_bh_on_exit' EXIT; exit 3 ) 2>&1 \
        | sed -n 's/^ *//; /^bh-spec-none:/p')"
expect 'a Ctrl+C stop does not' '' \
    "$( ( BH_EXTERNAL_TOOLS=bh-spec-none; trap '_bh_on_exit' EXIT; exit 130 ) 2>&1)"
rm -rf "$_bh_spec_sandbox"

test_section 'bh_qmake_trace -- --very-verbose tells what the qmake check compared'

# A source folder with an older and a newer `.pro`, a build folder whose
# Makefile sits between them, and a link to the source folder; the files'
# times are set by hand. The trace goes to stderr; the times come from
# `bh_file_time`, hence the expected lines name them through it too.
_bh_spec_sandbox=$(mktemp -d)
mkdir -p "$_bh_spec_sandbox/src/sub" "$_bh_spec_sandbox/bd"
: > "$_bh_spec_sandbox/src/top.pro"
: > "$_bh_spec_sandbox/src/sub/sub.pro"
: > "$_bh_spec_sandbox/bd/Makefile"
touch -t 199901010000 "$_bh_spec_sandbox/src/top.pro"
touch -t 200001010000 "$_bh_spec_sandbox/bd/Makefile"
touch -t 200101010000 "$_bh_spec_sandbox/src/sub/sub.pro"
ln -s src "$_bh_spec_sandbox/linked"
_bh_spec_mf=$_bh_spec_sandbox/bd/Makefile
_bh_spec_sub=$_bh_spec_sandbox/src/sub/sub.pro
_bh_spec_trace() {
    ( BH_ROOT=$_bh_spec_sandbox/$1 BUILD_DIR=$_bh_spec_sandbox/bd
      BH_LOG_LEVEL=$2 _label=root-project _bhq_makefile=Makefile
      _bhq_build_differs=0 BH_IGNORE_PRI=0
      cd "$_bh_spec_sandbox/bd" && shift 2 && "$@" 2>&1 >/dev/null )
}
expect 'nothing below --very-verbose' '' \
    "$(_bh_spec_trace src 2 bh_qmake_trace_newer "$_bh_spec_sub")"
expect 'the newer file, with both times' \
    "qmake check: Looks under $_bh_spec_sandbox/src for a .pro, .pri, or .prf newer than
qmake check: $_bh_spec_mf ($(bh_file_time "$_bh_spec_mf")):
qmake check:   found $_bh_spec_sub ($(bh_file_time "$_bh_spec_sub"))." \
    "$(_bh_spec_trace src 3 bh_qmake_trace_newer "$_bh_spec_sub")"
expect 'that none is newer' 'qmake check:   none.' \
    "$(_bh_spec_trace src 3 bh_qmake_trace_newer '' | tail -1)"
expect 'a source folder that is no link' 'qmake check:   '"$_bh_spec_sandbox"'/src is a link: no' \
    "$(_bh_spec_trace src 3 bh_qmake_trace_inputs | grep 'is a link')"
expect 'a source folder that is a link, and where it points' \
    'qmake check:   '"$_bh_spec_sandbox"'/linked is a link: yes, to src' \
    "$(_bh_spec_trace linked 3 bh_qmake_trace_inputs | grep 'is a link')"
expect 'the time of a missing file' missing "$(bh_file_time "$_bh_spec_sandbox/none")"
expect 'the time of a file, as stat tells it' \
    "$(stat -c '%y' "$_bh_spec_sub")" "$(bh_file_time "$_bh_spec_sub")"

test_section 'bh_newer_project_file -- a source folder that is a link'

expect 'the newer .pro, through the link' "$_bh_spec_sandbox/linked/sub/sub.pro" \
    "$(BH_ROOT=$_bh_spec_sandbox/linked; bh_newer_project_file "$_bh_spec_mf")"
rm -rf "$_bh_spec_sandbox"

test_section 'bh_make_plan -- the dry run walks the sub-makes of jom and nmake'

# A stub jom prints each line of its `-f` file (`Makefile` by default) after a
# tab, as jom's dry run prints each command, and runs none of them; the files
# hold the lines a dry run of qmake's Makefiles prints.
_bh_spec_sandbox=$(mktemp -d)
mkdir -p "$_bh_spec_sandbox/bd/src/corelib" "$_bh_spec_sandbox/bd/src/angle"
cat > "$_bh_spec_sandbox/jom" <<'}'
#!/bin/sh
file=Makefile
while [ "$#" -gt 0 ]; do
    case "$1" in
        -f) file=$2; shift 2 ;;
        *) shift ;;
    esac
done
sed 's/^/	/' "$file"
}
chmod +x "$_bh_spec_sandbox/jom"
_bh_spec_jom=$_bh_spec_sandbox/jom
printf '%s\n' \
    "cd src\\corelib\\ && ( if not exist Makefile qmake.exe corelib.pro -o Makefile ) && $_bh_spec_jom -f Makefile" \
    "cd src\\angle\\ && $_bh_spec_jom -f Makefile" > "$_bh_spec_sandbox/bd/Makefile"
printf '%s\n' "$_bh_spec_jom -f Makefile.Debug all" \
    > "$_bh_spec_sandbox/bd/src/corelib/Makefile"
printf '%s\n' 'cl -c -nologo a.cpp' 'cl -c -nologo b.cpp' 'link /DLL a.obj b.obj' \
    > "$_bh_spec_sandbox/bd/src/corelib/Makefile.Debug"
printf '%s\n' 'set MAKEFLAGS=x' 'cl -c x.cpp' > "$_bh_spec_sandbox/bd/src/angle/Makefile"
# _bh_spec_plan [BH_MAKE_PLAN_DEPTH]
# Prints how many build steps the stub's dry run shows from the top folder.
_bh_spec_plan() {
    ( cd "$_bh_spec_sandbox/bd" && BH_MAKE=$_bh_spec_jom BH_MAKE_TYPE=jom \
        BH_MAKE_PLAN_DEPTH=${1:-32} bh_make_plan | grep -cE "$BH_BUILD_STEP_RE" )
}
expect 'the steps of every sub-make, two levels deep' 4 "$(_bh_spec_plan)"
expect 'no deeper than BH_MAKE_PLAN_DEPTH' 1 "$(_bh_spec_plan 1)"
expect 'a sub-make line, after cd and a qmake call' "src\\corelib\\
-f
Makefile" \
    "$(printf '\tcd src\\corelib\\ && ( if not exist Makefile q -o Makefile ) && D:\\b\\jom.exe -f Makefile\n' \
        | bh_make_sub_call)"
expect 'a quoted tool, in the current folder' "
-f
Makefile.Debug
all" \
    "$(printf '\t"C:\\Program Files\\jom.exe" -f Makefile.Debug all\n' | bh_make_sub_call)"
expect 'a line that calls no sub-make' '' \
    "$(printf '\tcl -c -nologo a.cpp\n' | bh_make_sub_call)"
expect 'a step that jom prints after a tab counts' 1 \
    "$(printf '\tcl -c -nologo a.cpp\n' | grep -cE "$BH_BUILD_STEP_RE")"
_bh_spec_plan_file=$_bh_spec_sandbox/plan
printf '\tcl -c a.cpp\n\tcl -c b.cpp\n' > "$_bh_spec_plan_file"
expect 'the steps of a plan file' 2 "$(bh_make_plan_count "$_bh_spec_plan_file")"
expect 'no steps in a missing plan file' 0 "$(bh_make_plan_count "$_bh_spec_sandbox/none")"
: > "$_bh_spec_sandbox/stop"
expect 'a plan walks no more once its stop file exists' 0 \
    "$( ( cd "$_bh_spec_sandbox/bd" && BH_MAKE=$_bh_spec_jom BH_MAKE_TYPE=jom \
        BH_MAKE_PLAN_STOP=$_bh_spec_sandbox/stop bh_make_plan \
        | grep -cE "$BH_BUILD_STEP_RE" ) )"
expect 'a plan walks no more once its owner ended' 0 \
    "$( ( sh -c 'exit 0' & _bh_spec_gone=$!; wait "$_bh_spec_gone"
          cd "$_bh_spec_sandbox/bd" && BH_MAKE=$_bh_spec_jom BH_MAKE_TYPE=jom \
          BH_MAKE_PLAN_OWNER=$_bh_spec_gone bh_make_plan \
          | grep -cE "$BH_BUILD_STEP_RE" ) )"
test_section 'bh_progress_row -- a step row wraps, each further line below the description'

# _bh_spec_row <width> <total> <step> <description>
# Prints the row of a step at <width> columns, then its count of lines.
_bh_spec_row() {
    ( _bpr_width=$1 _bpr_last_key= BH_STEPS_TOTAL=$2
      bh_progress_row "$3" "$4"
      printf '[/]%s|%s' "$BH_PROGRESS_ROW" "$BH_PROGRESS_ROW_LINES" )
}
expect 'a row that fits' '[/] (1/3): Short.|1' "$(_bh_spec_row 80 3 1 'Short.')"
expect 'wraps between words, padded as wide as the prefix' '[/] (1/3): My long message
           that goes on here.|2' "$(_bh_spec_row 30 3 1 'My long message that goes on here.')"
expect 'a longer counter pads further' '[/] (1/345): My long message
             that goes on here.|2' \
    "$(_bh_spec_row 31 345 1 'My long message that goes on here.')"
expect 'a word longer than a line stands alone' '[/] (1/3): a
           unbreakable-word-here
           b|3' "$(_bh_spec_row 20 3 1 'a unbreakable-word-here b')"
expect 'no counter without a total' '[/]: My long
     message|2' "$(_bh_spec_row 14 '' 1 'My long message')"
expect 'no description' '[/] (1/3)|1' "$(_bh_spec_row 80 3 1 '')"
unset -f _bh_spec_row

test_section 'bh_progress_plan_start -- the modes of BH_PROGRESS_MODE'

# _bh_spec_mode [BH_PROGRESS_MODE]
# Prints the mode that a BH_PROGRESS_MODE resolves to, then what it warned.
_bh_spec_mode() {
    ( BH_PROGRESS_MODE=$1 _bhpp_mode_warned=0
      bh_progress_mode_resolve 2>"$_bh_spec_sandbox/warned"
      printf '%s|%s' "$_bhpp_mode" "$(grep -c BH_PROGRESS_MODE "$_bh_spec_sandbox/warned")" )
}
expect 'precise when unset' 'precise|0' "$(_bh_spec_mode '')"
expect 'spinner' 'spinner|0' "$(_bh_spec_mode spinner)"
expect 'estimate' 'estimate|0' "$(_bh_spec_mode estimate)"
expect 'precise' 'precise|0' "$(_bh_spec_mode precise)"
expect 'another value warns, and counts as precise' 'precise|1' \
    "$(_bh_spec_mode jump-back)"

# _bh_spec_start <mode> [code]
# Starts the build plan of the stub tree in <mode>, waits for a plan that runs
# beside the build to end, then runs [code]; prints the type the compile step
# starts with, its maximum unless a plan runs beside the build (whose count by
# then is a matter of timing), and what [code] prints.
_bh_spec_start() {
    ( cd "$_bh_spec_sandbox/bd" && BH_MAKE=$_bh_spec_jom BH_MAKE_TYPE=jom
      BH_PROGRESS_MODE=$1 BH_NO_PROGRESS=1 _step=0
      bh_progress_plan_start make "$_bh_spec_sandbox/plan" 2>/dev/null
      printf '%s' "$BH_PLAN_TYPE"
      if [ -n "$_bhpp_pid" ]; then
          wait "$_bhpp_pid"
      else
          printf '|%s' "$BH_PLAN_MAX"
      fi
      eval "${2:-}" )
}
expect 'spinner skips the plan' 'spinner|0|no plan' \
    "$(_bh_spec_start spinner '[ -f "$_bh_spec_sandbox/plan" ] || printf "|no plan"')"
expect 'precise counts the whole plan first, taking a step' 'bar|4|1|3' \
    "$( ( BH_STEPS_TOTAL=3; _bh_spec_start precise 'printf "|%s|%s" "$_step" "$BH_STEPS_TOTAL"' ) )"
expect 'estimate starts a bar of BH_ESTIMATED_ACTIONS, and takes no step' \
    'bar|0|3' \
    "$( ( BH_ESTIMATED_ACTIONS=9 BH_STEPS_TOTAL=3
          _bh_spec_start estimate 'printf "|%s|%s" "$_step" "$BH_STEPS_TOTAL"' ) )"
expect 'estimate spins with no BH_ESTIMATED_ACTIONS' 'spinner' \
    "$( ( BH_ESTIMATED_ACTIONS=; _bh_spec_start estimate ) )"
expect 'the tick after a hidden plan turns a spinner into a bar of its count' \
    'spinner|bar|4|4' \
    "$( ( BH_ESTIMATED_ACTIONS=; _bh_spec_start estimate 'bh_progress_plan_tick
        printf "|%s|%s|%s" "$BH_PROGRESS_TYPE" "$BH_PROGRESS_MAX" \
            "$(cat "$_bh_spec_sandbox/plan.bar")"' ) )"
expect 'the tick sets the bar of an estimate to the count' 'bar|4' \
    "$( ( BH_ESTIMATED_ACTIONS=9; _bh_spec_start estimate 'BH_PROGRESS_TYPE=bar
        BH_PROGRESS_MAX=9; bh_progress_plan_tick; printf "|%s" "$BH_PROGRESS_MAX"' ) )"
expect 'the finish hands the bar on to the shell the ticks ran beside' \
    'spinner|bar|4|1|' \
    "$( ( BH_ESTIMATED_ACTIONS=; _bh_spec_start estimate '( bh_progress_plan_tick )
        bh_progress_plan_finish
        printf "|%s|%s|%s|%s" "$BH_PROGRESS_TYPE" "$BH_PROGRESS_MAX" \
            "$BH_PROGRESS_HEADER_PRINTED" "$(ls "$_bh_spec_sandbox" | grep "^plan")"' ) )"
expect 'the exit hook stops a plan that still counts' 'bar|stopped' \
    "$( ( BH_ESTIMATED_ACTIONS=9; _bh_spec_start estimate 'rm -f "$_bh_spec_sandbox/plan.stop"
        sleep 30 & _bhpp_pid=$!
        bh_progress_plan_on_exit
        wait "$_bhpp_pid" 2>/dev/null
        [ "$?" -ne 0 ] && [ -f "$_bh_spec_sandbox/plan.stop" ] && printf "|stopped"' ) )"
rm -f "$_bh_spec_sandbox/plan.stop"

expect 'it tells a split build' 'split|' \
    "$( ( printf 'release-all: x\ndebug-all: y\n' > "$_bh_spec_sandbox/split.mk"
          : > "$_bh_spec_sandbox/one.mk"
          bh_make_is_split "$_bh_spec_sandbox/split.mk" && printf split
          bh_make_is_split "$_bh_spec_sandbox/one.mk" && printf one
          printf '|' ) )"

# The whole step in each mode ends fine, and leaves no plan file behind.
mkdir -p "$_bh_spec_sandbox/logs"
for _bh_spec_each in spinner estimate precise; do
    expect "a whole step under $_bh_spec_each" '0|' \
        "$( ( cd "$_bh_spec_sandbox/bd" && BH_MAKE=$_bh_spec_jom BH_MAKE_TYPE=jom \
              LOGS=$_bh_spec_sandbox/logs BH_PROGRESS_MODE=$_bh_spec_each \
              BH_NO_PROGRESS=1 _step=0
              bh_make_step make '' 2 >/dev/null 2>&1
              printf '%s|%s' "$?" "$(ls "$_bh_spec_sandbox/logs" | grep precount)" ) )"
done
rm -f "$_bh_spec_sandbox"/logs/actions-*

# _bh_spec_note <performed> [estimated] [filter]
# Records build actions as one make run would, then prints the note that the
# run ends with; the start and the end are 75 seconds apart.
_bh_spec_note() {
    ( LOGS=$_bh_spec_sandbox/logs BH_FILTER=${3:-} BH_REBUILD= BH_EXIT_STATUS=0
      BH_LOG_NOTES=1
      printf '\tcl -c a.cpp\n' > "$_bh_spec_sandbox/one.log"
      _bh_spec_i=0
      while [ "$_bh_spec_i" -lt "$1" ]; do
          bh_progress_actions_performed "$_bh_spec_sandbox/one.log"
          _bh_spec_i=$((_bh_spec_i + 1))
      done
      if [ -n "${2:-}" ]; then
          bh_progress_actions_estimated "$2"
      fi
      echo 1000 > "$(bh_progress_actions_file start)"
      echo 1075 > "$(bh_progress_actions_file end)"
      bh_progress_actions_note 2>&1
      ls "$_bh_spec_sandbox/logs" | grep '^actions-' )
}
expect 'the note, with the estimate' \
    'Note: performed 3 build actions in 00:01:15 duration
   (while estimated 9).' "$(_bh_spec_note 3 9)"
expect 'the note of a filtered build leaves the estimate out' \
    'Note: performed 3 build actions in 00:01:15 duration.' "$(_bh_spec_note 3 9 corelib)"
expect 'the note with no estimate' \
    'Note: performed 2 build actions in 00:01:15 duration.' "$(_bh_spec_note 2)"
expect 'no note under BH_LOG_NOTES=0' '' \
    "$( ( LOGS=$_bh_spec_sandbox/logs BH_EXIT_STATUS=0 BH_LOG_NOTES=0
          bh_progress_actions_performed "$_bh_spec_sandbox/one.log"
          bh_progress_actions_note 2>&1
          ls "$_bh_spec_sandbox/logs" | grep '^actions-' ) )"
expect 'no note after a failure' '' \
    "$( ( LOGS=$_bh_spec_sandbox/logs BH_EXIT_STATUS=2 BH_LOG_NOTES=1
          bh_progress_actions_performed "$_bh_spec_sandbox/one.log"
          bh_progress_actions_note 2>&1
          ls "$_bh_spec_sandbox/logs" | grep '^actions-' ) )"
expect 'the --estimate-actions flag' 'estimate|120' \
    "$( ( bh_parse_args --estimate-actions=120 >/dev/null 2>&1
          printf '%s|%s' "$BH_PROGRESS_MODE" "$BH_ESTIMATED_ACTIONS" ) )"
unset -f _bh_spec_note
rm -rf "$_bh_spec_sandbox"
unset -f _bh_spec_plan _bh_spec_mode _bh_spec_start

test_section 'bh_command_variant -- the one build variant of a qmake call'

expect 'debug' debug "$(bh_command_variant \
    'qmake CONFIG+=no_debug_and_release CONFIG-=release CONFIG+=debug x.pro')"
expect 'release' release "$(bh_command_variant \
    'qmake CONFIG+=no_debug_and_release CONFIG-=debug CONFIG+=release -o Makefile')"
expect 'release with debug info' release-debug-info "$(bh_command_variant \
    'qmake CONFIG+=no_debug_and_release CONFIG+=release CONFIG+=force_debug_info')"
expect 'nothing without no_debug_and_release' '' \
    "$(bh_command_variant 'qmake CONFIG+=debug x.pro')"
expect 'a later CONFIG+= wins, as in qmake' release "$(bh_command_variant \
    'qmake CONFIG+=no_debug_and_release CONFIG+=debug CONFIG+=release')"
expect 'a later CONFIG-= drops the variant' '' "$(bh_command_variant \
    'qmake CONFIG+=no_debug_and_release CONFIG+=debug CONFIG-=debug')"
_bh_spec_sandbox=$(mktemp -d)
: > "$_bh_spec_sandbox/CONFIG+=release"
expect 'an argument such as * stays a word, never a file name' debug \
    "$(cd "$_bh_spec_sandbox" && bh_command_variant \
        'qmake CONFIG+=no_debug_and_release CONFIG+=debug *')"
rm -rf "$_bh_spec_sandbox"

test_section 'bh_version_at_least -- dotted versions compare part by part'

expect_returns 'a version equal to the minimum is enough' \
    0 bh_version_at_least 1.0.16 1.0.16
expect_returns 'a newer minor part is enough, whatever the patch part' \
    0 bh_version_at_least 1.1.0 1.0.16
expect_returns 'parts compare as numbers, not as text' \
    1 bh_version_at_least 1.0.9 1.0.16
expect_returns 'an older patch part is not enough' \
    1 bh_version_at_least 1.0.13 1.0.16
expect_returns 'a missing part counts as 0' \
    0 bh_version_at_least 1.0 1.0.0
expect_returns 'a missing part is older than a nonzero one' \
    1 bh_version_at_least 1.0 1.0.1
expect_returns 'a newer major part is enough' \
    0 bh_version_at_least 2 1.9.9


# ---- specs: bh_default_jobs / bh_default_qmakespec --------------------

test_section 'bh_default_jobs / bh_default_qmakespec'

unset JOBS
bh_default_jobs
expect_int_gt 'JOBS picks a positive integer' "$JOBS" 0

# QMAKESPEC depends on host: Darwin -> macx-clang, otherwise linux-g++.
unset QMAKESPEC
bh_default_qmakespec
case "$(uname -s)" in
    Darwin) expect 'QMAKESPEC on Darwin' macx-clang "$QMAKESPEC";;
    *)      expect 'QMAKESPEC on non-Darwin' linux-g++ "$QMAKESPEC";;
esac

# Pre-set QMAKESPEC should be preserved.
QMAKESPEC=some-custom-spec
bh_default_qmakespec
expect 'env-preset QMAKESPEC survives' some-custom-spec "$QMAKESPEC"


# ---- specs: bh_yes_no -------------------------------------------------

test_section 'bh_warning -- shared stderr warning with bold header and indent'

# Regression guard for bh_warning's shape: leading `\n`, bold `WARNING:`
# (tty-only), 9-space follow-up indent, trailing `\n`.

_bh_spec_warn_body=$(awk '/^bh_warning\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'bh_warning gates the bold escape on `[ -t 2 ]`' \
    "$(printf '%s\n' "$_bh_spec_warn_body" | grep -cE '\[ -t 2 \]')" 0
expect_int_gt 'bh_warning uses \\033[1m to open the bold span' \
    "$(printf '%s\n' "$_bh_spec_warn_body" | grep -cF '\033[1m')" 0
expect_int_gt 'bh_warning uses \\033[22m to close the bold span (not full reset)' \
    "$(printf '%s\n' "$_bh_spec_warn_body" | grep -cF '\033[22m')" 0
expect_int_gt 'bh_warning starts the header line with a leading \\n' \
    "$(printf '%s\n' "$_bh_spec_warn_body" | grep -cE "'\\\\n%sWARNING:%s")" 0
expect_int_gt 'bh_warning ends the block with a trailing blank \\n' \
    "$(printf '%s\n' "$_bh_spec_warn_body" | grep -cF "printf '\\n' 1>&2")" 0
expect_int_gt 'bh_warning indents follow-up lines 9 spaces' \
    "$(printf '%s\n' "$_bh_spec_warn_body" | grep -cE "'         %s\\\\n'")" 0


test_section 'bh_license_accept -- --headless uses sudo -n with warning'

# Regression guard: --headless must use `sudo -n` (non-interactive); fall
# back to bh_warning on failure. Interactive path keeps plain `sudo`.

_bh_spec_lic_body=$(awk '/^bh_license_accept\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'bh_license_accept gates on --headless (BH_HEADLESS)' \
    "$(printf '%s\n' "$_bh_spec_lic_body" | grep -cE 'BH_HEADLESS')" 0
expect_int_gt 'bh_license_accept uses `sudo -n` under --headless' \
    "$(printf '%s\n' "$_bh_spec_lic_body" | grep -cE 'sudo -n xcodebuild')" 0
expect_int_gt 'bh_license_accept delegates to `bh_warning`' \
    "$(printf '%s\n' "$_bh_spec_lic_body" | grep -cE 'bh_warning')" 0
expect 'bh_license_accept does NOT re-implement the WARNING layout' '' \
    "$(printf '%s\n' "$_bh_spec_lic_body" | grep -F 'WARNING:' | head -1)"
expect_int_gt 'bh_license_accept keeps plain `sudo` on the interactive path' \
    "$(printf '%s\n' "$_bh_spec_lic_body" | grep -cE '^[[:space:]]*sudo xcodebuild')" 0


test_section 'bh_license_gate -- names the Xcode license on macOS only'

# Prints the question the gate asks on a host whose `uname -s` is <name>,
# with the prompt, the message and the license step stubbed out.
_bh_spec_gate_question() (
    eval "uname() { echo $1; }"
    bh_yes_no() { echo "$1"; }
    bh_license_message() { :; }
    bh_license_accept() { :; }
    BH_HEADLESS=0
    bh_license_gate
)

expect 'on Windows the gate asks about the build directory alone' \
    'Continue with this build directory? [Y/n]' \
    "$(_bh_spec_gate_question MINGW64_NT-10.0-19045)"
expect 'on Linux the gate asks about the build directory alone' \
    'Continue with this build directory? [Y/n]' \
    "$(_bh_spec_gate_question Linux)"
expect 'on macOS the gate also asks to accept the Xcode license' \
    'Continue with this build directory and accept the Xcode license? [Y/n]' \
    "$(_bh_spec_gate_question Darwin)"


test_section 'bh_yes_no -- under --headless'

BH_HEADLESS=1
# Under --headless, bh_yes_no should auto-yes (return 0) without
# reading stdin.
expect_returns 'bh_yes_no auto-accepts under --headless' 0 \
    bh_yes_no 'irrelevant prompt'


# ---- specs: bh_progress_* -- state-only ------------------------------

test_section 'bh_progress_* -- state setters'

# Reset BH_PROGRESS_* tracker vars.
unset BH_PROGRESS_MAX BH_PROGRESS_VALUE BH_PROGRESS_DONE
unset BH_PROGRESS_ARMED BH_PROGRESS_DESC BH_PROGRESS_TYPE
unset BH_PROGRESS_HEADER_PRINTED

bh_progress_setMax 42
expect 'setMax stores BH_PROGRESS_MAX'   42 "$BH_PROGRESS_MAX"
expect 'setMax resets BH_PROGRESS_VALUE' 0  "$BH_PROGRESS_VALUE"
expect 'setMax clears BH_PROGRESS_DONE'  0  "$BH_PROGRESS_DONE"
expect 'setMax arms BH_PROGRESS_ARMED'   1  "$BH_PROGRESS_ARMED"

bh_progress_setValue 17
expect 'setValue stores BH_PROGRESS_VALUE' 17 "$BH_PROGRESS_VALUE"

bh_progress_setDescription 'hello world'
expect 'setDescription stores BH_PROGRESS_DESC' 'hello world' "$BH_PROGRESS_DESC"

bh_progress_setType bar
expect 'setType stores BH_PROGRESS_TYPE'           bar "$BH_PROGRESS_TYPE"
expect 'setType resets BH_PROGRESS_HEADER_PRINTED' 0   "$BH_PROGRESS_HEADER_PRINTED"

bh_progress_setType spinner
expect 'setType switches mode'                      spinner "$BH_PROGRESS_TYPE"

test_section 'bh_progress_format_bracket -- bar visualisation'

# These tests exercise the format helper directly -- it has no
# TTY/no-progress gating, returns the inside-the-brackets string,
# and is the seam the embedded-spinner behaviour depends on.

# State helper: reset the format-affecting vars to known defaults.
_bh_spec_fmt_reset() {
    unset BH_PROGRESS_TICK BH_PROGRESS_VALUE BH_PROGRESS_MAX
    unset BH_PROGRESS_TYPE BH_PROGRESS_DONE
}

# Spinner mode -- the bracket content is a single rotating char.
_bh_spec_fmt_reset
BH_PROGRESS_TYPE=spinner
BH_PROGRESS_MAX=0
BH_PROGRESS_TICK=0
expect 'spinner mode @ tick 0 returns |' \
    '|' "$(bh_progress_format_bracket)"

BH_PROGRESS_TICK=1
expect 'spinner mode @ tick 1 returns /' \
    '/' "$(bh_progress_format_bracket)"

BH_PROGRESS_TICK=2
expect 'spinner mode @ tick 2 returns -' \
    '-' "$(bh_progress_format_bracket)"

BH_PROGRESS_TICK=3
expect 'spinner mode @ tick 3 returns \\' \
    '\' "$(bh_progress_format_bracket)"

BH_PROGRESS_TICK=4
expect 'spinner mode wraps back to |' \
    '|' "$(bh_progress_format_bracket)"

# Done state -- overrides both modes.
_bh_spec_fmt_reset
BH_PROGRESS_TYPE=spinner
BH_PROGRESS_DONE=1
expect 'done state returns √ (spinner mode)' \
    '√' "$(bh_progress_format_bracket)"

BH_PROGRESS_TYPE=bar
BH_PROGRESS_MAX=10
expect 'done state returns √ (bar mode)' \
    '√' "$(bh_progress_format_bracket)"

# Bar mode -- the embedded spinner sits at the bar's leading edge.
# This is the case the user complained about for already-built
# repos: when V=M (no-op), the bar should still expose the spinner
# character so the rotating frame is visible.
_bh_spec_fmt_reset
BH_PROGRESS_TYPE=bar
BH_PROGRESS_MAX=1
BH_PROGRESS_VALUE=1
BH_PROGRESS_TICK=1
expect 'bar @ V=1/M=1 (no-op): 39 hashes then spinner /' \
    '#######################################/' \
    "$(bh_progress_format_bracket)"

BH_PROGRESS_TICK=2
expect 'bar @ V=1/M=1 with next tick: 39 hashes then -' \
    '#######################################-' \
    "$(bh_progress_format_bracket)"

# Bar @ 0% -- spinner at position 0, 39 underscores after.
_bh_spec_fmt_reset
BH_PROGRESS_TYPE=bar
BH_PROGRESS_MAX=100
BH_PROGRESS_VALUE=0
BH_PROGRESS_TICK=0
expect 'bar @ V=0/M=100: spinner then 39 underscores' \
    '|_______________________________________' \
    "$(bh_progress_format_bracket)"

# Bar @ 50% -- 20 hashes + spinner + 19 underscores.
_bh_spec_fmt_reset
BH_PROGRESS_TYPE=bar
BH_PROGRESS_MAX=100
BH_PROGRESS_VALUE=50
BH_PROGRESS_TICK=1
expect 'bar @ V=50/M=100: 20 hashes + / + 19 underscores' \
    '####################/___________________' \
    "$(bh_progress_format_bracket)"

# Bar length is always 40 -- regression guard against accidentally
# changing the width without updating the centi-percent divisor.
_bh_spec_fmt_reset
BH_PROGRESS_TYPE=bar
BH_PROGRESS_MAX=2466
BH_PROGRESS_VALUE=745
BH_PROGRESS_TICK=0
_bh_spec_bracket=$(bh_progress_format_bracket)
expect 'bar width is exactly 40 chars (regression guard)' \
    40 "${#_bh_spec_bracket}"


test_section 'bh_progress_render / hide -- TTY-gated no-op'

# bh_progress_render is a no-op when stderr isn't a TTY. The script
# is running under shell redirection here, so [ -t 2 ] is false --
# `render` returns 0 without emitting anything.
expect_returns 'bh_progress_render is a no-op when stderr is non-TTY' 0 \
    bh_progress_render

# bh_progress_hide should also no-op when stderr is non-TTY (it
# bails before printing). Even with ARMED=1 it returns 0.
BH_PROGRESS_ARMED=1
expect_returns 'bh_progress_hide is a no-op when stderr is non-TTY' 0 \
    bh_progress_hide

# BH_NO_PROGRESS=1 also short-circuits render. Test with ARMED=1
# and a TTY-style env (cannot actually fake stderr, but the
# return-0 path runs first).
BH_NO_PROGRESS=1
expect_returns 'BH_NO_PROGRESS=1 short-circuits render' 0 \
    bh_progress_render
unset BH_NO_PROGRESS


# ---- specs: bh_strict_mode -------------------------------------------

test_section 'bh_make_step -- KEEPALIVE keeps embedded spinner spinning'

# Regression guard: KEEPALIVE ticker + awk `K` sentinel + shell `K)` branch
# must remain; without all three, the spinner freezes at low CPU.

# Hoisted: also reused by the "ticker cleanup tolerates set -e"
# section below.
_bh_spec_makestep_body=$(awk '/^bh_make_step\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'ticker writes `KEEPALIVE` lines' \
    "$(printf '%s\n' "$_bh_spec_makestep_body" | grep -cF "printf 'KEEPALIVE")" 0
expect_int_gt 'awk converts `KEEPALIVE` -> `K` sentinel (before verbose catch-all)' \
    "$(printf '%s\n' "$_bh_spec_makestep_body" | grep -cE '/\^KEEPALIVE\$/.*print "K"')" 0
expect_int_gt 'shell loop has a `K)` spinner-tick branch' \
    "$(printf '%s\n' "$_bh_spec_makestep_body" | grep -cE '^[[:space:]]+K\)$')" 0


test_section 'bh_make_step -- ticker cleanup tolerates set -e'

# Regression guard: ticker `kill` + `wait` must be `|| true`-guarded so a
# race exit doesn't trip set -e and abort before `bh_progress_hide`.

expect_int_gt 'ticker `kill` is `|| true`-guarded' \
    "$(printf '%s\n' "$_bh_spec_makestep_body" | grep -cE 'kill .*_bh_ticker_pid.*\|\| true')" 0
expect_int_gt 'ticker `wait` is `|| true`-guarded' \
    "$(printf '%s\n' "$_bh_spec_makestep_body" | grep -cE 'wait .*_bh_ticker_pid.*\|\| true')" 0


test_section 'bh_progress_hide -- always emits \\n after the [√] render'

# Regression guard: hide must set DONE=1, repaint, and end with a stderr
# `\n` -- otherwise zsh shows its `%` PROMPT_EOL_MARK.

_bh_spec_hide_body=$(awk '/^bh_progress_hide\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'hide sets BH_PROGRESS_DONE=1' \
    "$(printf '%s\n' "$_bh_spec_hide_body" | grep -c 'BH_PROGRESS_DONE=1')" 0
expect_int_gt 'hide calls bh_progress_render to repaint the row' \
    "$(printf '%s\n' "$_bh_spec_hide_body" | grep -c 'bh_progress_render')" 0
expect_int_gt 'hide trails with a stderr newline (no trailing %% sign)' \
    "$(printf '%s\n' "$_bh_spec_hide_body" | grep -cF "printf '\\n' 1>&2")" 0


test_section 'bh_progress_render -- bar DONE collapses two rows into one with description'

# Regression guard: bar mode + DONE must collapse the two-row layout
# into one `[√] (N/M): description` row via `\033[1A` (cursor-up).

_bh_spec_render_body=$(awk '/^bh_progress_render\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'bar DONE prints `: description`' \
    "$(printf '%s\n' "$_bh_spec_render_body" | grep -cF 'bh_progress_row "${_step:-}" "${BH_PROGRESS_DESC:-}"')" 1
expect_int_gt 'bar DONE moves cursor up over the header row (\\033[1A)' \
    "$(printf '%s\n' "$_bh_spec_render_body" | grep -cF '\033[%dA')" 0
expect_int_gt 'bar DONE resets BH_PROGRESS_HEADER_PRINTED' \
    "$(printf '%s\n' "$_bh_spec_render_body" | grep -cE 'BH_PROGRESS_HEADER_PRINTED=0')" 0


test_section 'bh_run_qmake -- freshness check walks .pro / .pri / .prf'

# Regression guard: freshness check walks `.pro` + `.pri` + `.prf` by
# default; `--ignore-pri` narrows back to just `.pro`.

_bh_spec_runqmake_body=$(awk '/^bh_run_qmake\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")
_bh_spec_newer_body=$(awk '/^bh_newer_project_file\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'freshness check goes through bh_newer_project_file' \
    "$(printf '%s\n' "$_bh_spec_runqmake_body" | grep -c 'bh_newer_project_file "$_bhq_makefile"')" 0
expect_int_gt 'freshness `find` looks for `.pri` (default path)' \
    "$(printf '%s\n' "$_bh_spec_newer_body" | grep -cE "name '\\*\\.pri'")" 0
expect_int_gt 'freshness `find` looks for `.prf` (default path)' \
    "$(printf '%s\n' "$_bh_spec_newer_body" | grep -cE "name '\\*\\.prf'")" 0
expect_int_gt 'freshness check gates `.pri`/`.prf` walk on `BH_IGNORE_PRI`' \
    "$(printf '%s\n' "$_bh_spec_newer_body" | grep -cE 'BH_IGNORE_PRI')" 0


test_section '--rebuild -- --filter is the base filter, --rebuild the sub-filter'

# bh_filter_candidate sorts one path; bh_save_space_forget (a filtered
# --clean) drops the finished lines of what both filters name, and keeps
# every order line in its place.

_bh_spec_root=$BH_ROOT
_bh_spec_build_dir=${BUILD_DIR:-}
_bh_spec_sandbox=$(mktemp -d)
BH_ROOT=/r
BUILD_DIR=$_bh_spec_sandbox
_bh_spec_tab=$(printf '\t')

BH_FILTER='src/' BH_REBUILD='b'
bh_filter_candidate 'src/a/a.pro'
expect 'base-filter match, sub-filter miss -> build' build "$BH_CANDIDATE"
bh_filter_candidate 'src/b/b.pro'
expect 'base-filter match, sub-filter match -> rebuild' rebuild "$BH_CANDIDATE"
bh_filter_candidate 'tests/b/b.pro'
expect 'base-filter miss -> skip, even if the sub-filter matches' \
    skip "$BH_CANDIDATE"
BH_FILTER='' BH_REBUILD=''
bh_filter_candidate 'tests/x/x.pro'
expect 'no filters -> build' build "$BH_CANDIDATE"

# Writes a cache with two order lines and four finished lines.
_bh_spec_forget_cache() {
    printf '%s\n' \
        "/r/src/a/a.pro${_bh_spec_tab}debug${_bh_spec_tab}/t/a" \
        "/r/src/b/b.pro${_bh_spec_tab}debug${_bh_spec_tab}/t/b" \
        "/r/src/a/a.pro${_bh_spec_tab}debug${_bh_spec_tab}/t/a${_bh_spec_tab}T1" \
        "/r/src/b/b.pro${_bh_spec_tab}debug${_bh_spec_tab}/t/b${_bh_spec_tab}T2" \
        "/r/tests/x/x.pro${_bh_spec_tab}debug${_bh_spec_tab}/t/x${_bh_spec_tab}T3" \
        "/r/src/ab/ab.pro${_bh_spec_tab}debug${_bh_spec_tab}/t/ab${_bh_spec_tab}T4" \
        > "$BUILD_DIR/.qmake.save_space"
}

# Runs bh_save_space_forget under <filter> and <rebuild>, then prints the
# projects whose finished lines are left, relative to /r.
_bh_spec_forget() {
    _bh_spec_forget_cache
    BH_FILTER=$1 BH_REBUILD=$2
    bh_save_space_forget
    awk -F'\t' 'NF >= 4 { sub(/^\/r\//, "", $1); printf "%s ", $1 }' \
        "$BUILD_DIR/.qmake.save_space"
}

expect 'forget, base filter only: drops its matches' \
    'src/b/b.pro tests/x/x.pro src/ab/ab.pro ' "$(_bh_spec_forget 'src/a/' '')"
expect 'forget, both filters: drops only what both name' \
    'src/a/a.pro tests/x/x.pro ' "$(_bh_spec_forget 'src/' 'b')"
expect 'forget, sub-filter only: drops its matches' \
    'src/a/a.pro src/b/b.pro src/ab/ab.pro ' "$(_bh_spec_forget '' 'tests/')"
expect 'forget, bare --rebuild: drops every base-filter match' \
    'tests/x/x.pro ' "$(_bh_spec_forget 'src/' '^.*$')"
expect 'forget, no match: drops nothing' \
    'src/a/a.pro src/b/b.pro tests/x/x.pro src/ab/ab.pro ' \
    "$(_bh_spec_forget 'nomatch' '')"
_bh_spec_forget 'src/' '^.*$' > /dev/null
expect 'forget keeps every order line, in its place' \
    "/r/src/a/a.pro${_bh_spec_tab}debug${_bh_spec_tab}/t/a /r/src/b/b.pro${_bh_spec_tab}debug${_bh_spec_tab}/t/b" \
    "$(awk -F'\t' 'NF == 3' "$BUILD_DIR/.qmake.save_space" | tr '\n' ' ' | sed 's/ $//')"

# A variant recorded again with another target (a later qmake run in another
# build mode, or a renamed target) counts with its latest target only.
: > "$BUILD_DIR/new"
printf '%s\n' \
    "/r/src/c/c.pro${_bh_spec_tab}debug${_bh_spec_tab}$BUILD_DIR/old" \
    "/r/src/c/c.pro${_bh_spec_tab}debug${_bh_spec_tab}$BUILD_DIR/new" \
    "/r/src/c/c.pro${_bh_spec_tab}debug${_bh_spec_tab}$BUILD_DIR/new${_bh_spec_tab}T5" \
    > "$BUILD_DIR/.qmake.save_space"
expect_returns 'is_finished: the latest target of a re-recorded variant counts' \
    0 bh_save_space_is_finished /r/src/c/c.pro
expect_returns 'mark_finished: a superseded target of the variant is not demanded' \
    0 bh_save_space_mark_finished /r/src/c/c.pro "$BUILD_DIR/Makefile"
rm -f "$BUILD_DIR/new"
expect_returns 'mark_finished: the latest target missing still fails' \
    1 bh_save_space_mark_finished /r/src/c/c.pro "$BUILD_DIR/Makefile"

# After a switch from debug_and_release to a single config, the dual mode's
# lines (whose targets may be gone) no longer count.
: > "$BUILD_DIR/single"
printf '%s\n' \
    "/r/src/d/d.pro${_bh_spec_tab}debug_and_release:release${_bh_spec_tab}$BUILD_DIR/gone" \
    "/r/src/d/d.pro${_bh_spec_tab}debug_and_release:debug${_bh_spec_tab}$BUILD_DIR/goned" \
    "/r/src/d/d.pro${_bh_spec_tab}debug${_bh_spec_tab}$BUILD_DIR/single" \
    "/r/src/d/d.pro${_bh_spec_tab}debug${_bh_spec_tab}$BUILD_DIR/single${_bh_spec_tab}T6" \
    > "$BUILD_DIR/.qmake.save_space"
expect_returns 'is_finished: only the latest build mode counts' \
    0 bh_save_space_is_finished /r/src/d/d.pro
expect_returns 'mark_finished: targets of the other build mode are not demanded' \
    0 bh_save_space_mark_finished /r/src/d/d.pro "$BUILD_DIR/Makefile"

# Under --save-space, a build root made for another variant starts its cache
# anew before qmake records the order again; one of the same variant keeps the
# cache, and so does a run without --save-space. The qmake run itself is
# stubbed out.
printf '%s\n' '# Command: /xd/bin/qmake -spec /xd/mkspecs/linux-g++ CONFIG+=no_debug_and_release CONFIG-=debug CONFIG+=release x.pro' \
    > "$BUILD_DIR/Makefile"
# _bh_spec_record <save-space> <mode>: records the order the way a run with
# that --save-space setting and mode flag does, without running qmake.
_bh_spec_record() {
    ( QMAKESPEC=linux-g++; BH_SAVE_SPACE=$1; BH_FORCE_QMAKE=0; BH_MODE=$2
      BH_REMAINING_ARGS=$(bh_variant_qmake_args)
      bh_qmake_in_build_order() { :; }
      bh_build_order_record /r/x.pro > /dev/null )
}
_bh_spec_forget_cache
_bh_spec_record 1 debug
expect 'a --save-space --debug run over a release-only build root empties the cache' \
    0 "$(awk 'END { print NR }' "$BUILD_DIR/.qmake.save_space")"
_bh_spec_forget_cache
_bh_spec_record 1 release
expect 'a --save-space --release run over it keeps every line' \
    6 "$(awk 'END { print NR }' "$BUILD_DIR/.qmake.save_space")"
_bh_spec_forget_cache
_bh_spec_record 0 debug
expect 'a --debug run without --save-space keeps every line too' \
    6 "$(awk 'END { print NR }' "$BUILD_DIR/.qmake.save_space")"
unset -f _bh_spec_record
rm -f "$BUILD_DIR/Makefile"

rm -rf "$_bh_spec_sandbox"
BH_ROOT=$_bh_spec_root
BUILD_DIR=$_bh_spec_build_dir
unset BH_FILTER BH_REBUILD BH_CANDIDATE


test_section '--static -- a sub-filter of --filter, just like --rebuild'

unset BH_STATIC BH_NO_XD_BUILD
bh_parse_args
expect 'BH_STATIC is unset by default' '' "${BH_STATIC:-}"
expect 'BH_NO_XD_BUILD default is 0' 0 "$BH_NO_XD_BUILD"
bh_parse_args --static
expect '--static alone accepts every project' '^.*$' "$BH_STATIC"
bh_parse_args --static='src/(a|b)/'
expect '--static=<regexp> keeps its regexp' 'src/(a|b)/' "$BH_STATIC"
BH_STATIC='^.*$'
bh_parse_args --no-static
expect '--no-static clears a preset BH_STATIC' '' "$BH_STATIC"
BH_NO_XD_BUILD=0
bh_parse_args --no-xd-build
expect '--no-xd-build sets BH_NO_XD_BUILD=1' 1 "$BH_NO_XD_BUILD"
BH_NO_XD_BUILD=0
bh_parse_args --no-qt-build
expect '--no-qt-build is an alias of --no-xd-build' 1 "$BH_NO_XD_BUILD"
BH_NO_XD_BUILD=0

BH_FILTER='src/' BH_REBUILD='' BH_STATIC='b'
bh_filter_candidate 'src/a/a.pro'
expect 'base-filter match, static miss -> shared' \
    'build 0' "$BH_CANDIDATE $BH_CANDIDATE_STATIC"
bh_filter_candidate 'src/b/b.pro'
expect 'base-filter match, static match -> static' \
    'build 1' "$BH_CANDIDATE $BH_CANDIDATE_STATIC"
bh_filter_candidate 'tests/b/b.pro'
expect 'base-filter miss -> shared, even if --static matches' \
    'skip 0' "$BH_CANDIDATE $BH_CANDIDATE_STATIC"
BH_FILTER='' BH_REBUILD='b' BH_STATIC=''
bh_filter_candidate 'src/b/b.pro'
expect '--rebuild alone marks nothing static' \
    'rebuild 0' "$BH_CANDIDATE $BH_CANDIDATE_STATIC"
BH_FILTER='' BH_REBUILD='' BH_STATIC=''

# bh_xd_build and bh_xd_build_static against a fake XD whose build.sh records
# its arguments and the BUILD_DIR, BH_ROOT, and BH_HEADLESS it sees, then
# leaves its libraries behind: under --static a static QtCore and QtGui (only
# the module file of QtGui while a `no-gui` file exists), else a shared QtCore
# in the form the host takes (or `.dll` under --cross-compile) and the static
# library of a `staticlib` module.
_bh_spec_fake_xd=$(mktemp -d)
_bh_spec_xd_bd=$(mktemp -d)
cat > "$_bh_spec_fake_xd/build.sh" <<'EOF'
cd "${0%/*}" || exit 1
echo "$* [${BUILD_DIR:-}|${BH_ROOT:-}|${BH_HEADLESS:-}]" > args
case " $* " in
    *" --static "*)
        mkdir -p lib mkspecs/modules/static
        echo 'QT.core_static.module = Qt5Core' > mkspecs/modules/static/qt_lib_core.pri
        echo 'QT.gui_static.module = Qt5Gui' > mkspecs/modules/static/qt_lib_gui.pri
        echo 'QT.gui_private_static.module =' > mkspecs/modules/static/qt_lib_gui_private.pri
        : > lib/libQt5Core_static.a
        if [ ! -f no-gui ]; then
            : > lib/libQt5Gui_static.a
        fi
        ;;
    *)
        case " $*:$(uname -s)" in
            *" --cross-compile"* | *:MINGW* | *:MSYS* | *:CYGWIN*) ext=dll ;;
            *:Darwin) ext=dylib ;;
            *) ext=so ;;
        esac
        mkdir -p lib mkspecs/modules
        echo 'QT.core.module = Qt5Core' > mkspecs/modules/qt_lib_core.pri
        printf '%s\n' 'QT.bootstrap_private.module = Qt5Bootstrap' \
            'QT.bootstrap_private.module_config = v2 staticlib internal_module' \
            > mkspecs/modules/qt_lib_bootstrap_private.pri
        : > "lib/libQt5Core.$ext"
        : > lib/libQt5Bootstrap.a
        ;;
esac
EOF
# _bh_spec_xd_build [flags...]
# Parses the flags in a subshell, under the fake XD and an exported BUILD_DIR
# and BH_ROOT (which XD's build must not see), with no BH_HEADLESS unless
# `_bh_spec_headless_env` is set, and with a `.qmake.static` list
# that names one project under --static (the projects of
# `_bh_spec_static_list` when set) and none without, then runs
# bh_xd_build_static; prints what the fake build.sh recorded, or `none` when
# it did not run.
_bh_spec_xd_build() (
    rm -f "$_bh_spec_fake_xd/args"
    XD_DIR=$_bh_spec_fake_xd
    export BUILD_DIR=$_bh_spec_xd_bd BH_ROOT=/r
    unset BH_STATIC BH_NO_XD_BUILD BH_MODE BH_DEBUG_INFO BH_LOG_LEVEL BH_VERBOSE
    unset BH_NO_PROGRESS BH_SAVE_SPACE BH_NO_BUILD BH_TEMPLATE BH_CROSS_SPEC
    if [ -z "${_bh_spec_headless_env:-}" ]; then
        unset BH_HEADLESS
    fi
    bh_parse_args "$@"
    _bh_spec_names=${_bh_spec_static_list-/r/a/a.pro}
    if [ -n "${BH_STATIC:-}" ] && [ -n "$_bh_spec_names" ]; then
        printf '%s\n' "$_bh_spec_names" > "$BUILD_DIR/.qmake.static"
    else
        : > "$BUILD_DIR/.qmake.static"
    fi
    bh_xd_build_static > /dev/null
    cat "$_bh_spec_fake_xd/args" 2>/dev/null || echo none
)
# _bh_spec_no_gui
# Takes the static QtGui library away; hence XD lacks a whole static build
# again.
_bh_spec_no_gui() {
    rm -f "$_bh_spec_fake_xd/lib/libQt5Gui_static.a"
}
expect 'without --static, XD is not built in static mode' none \
    "$(_bh_spec_xd_build)"
expect 'a --static that names no project leaves XD alone' none \
    "$(_bh_spec_static_list=; _bh_spec_xd_build --static=nothing)"
expect '--no-xd-build leaves a non-static XD alone' none \
    "$(_bh_spec_xd_build --static --no-xd-build)"
expect '--static builds XD with a plain --static, without our BUILD_DIR' \
    '--static [||]' "$(_bh_spec_xd_build --static=app)"
expect 'and not again, once XD has a static build' none \
    "$(_bh_spec_xd_build --static)"
_bh_spec_no_gui
expect 'a static build missing one module library builds again' \
    '--static [||]' "$(_bh_spec_xd_build --static)"
_bh_spec_no_gui
expect 'every flag goes along as given, with its value' \
    '--static --release -v --no-progress --save-space --headless --get build-dir [||]' \
    "$(_bh_spec_xd_build --release -v --static --no-progress --save-space \
        --headless --get build-dir)"
_bh_spec_no_gui
expect 'so do --clean, --rebuild, --qmake, and the qmake arguments' \
    '--static --clean --rebuild=x --qmake --libc=musl --cross-compile -- CONFIG+=a b [||]' \
    "$(_bh_spec_xd_build --static --clean --rebuild=x --qmake --libc=musl \
        --cross-compile -- CONFIG+=a b)"
_bh_spec_no_gui
expect 'but no flag that names this project'"'"'s own paths or steps' \
    '--static --release [||]' \
    "$(_bh_spec_xd_build --filter src/ --only=src --static=app --test \
        --tests --run --release)"
_bh_spec_no_gui
expect '--test-review passes on its defaults only, ahead of the other flags' \
    '--static --headless --verbose --no-build --no-verbose --build [||]' \
    "$(_bh_spec_xd_build --static --no-verbose --test-review --build)"
_bh_spec_no_gui
expect 'a run that is not headless builds XD too, just without --headless' \
    '--static [||]' "$(_bh_spec_xd_build --static)"
_bh_spec_no_gui
expect 'an exported setting reaches XD'"'"'s build as well' \
    '--static [||1]' "$(export BH_HEADLESS=1; _bh_spec_headless_env=1
        _bh_spec_xd_build --static)"
_bh_spec_no_gui
: > "$_bh_spec_fake_xd/no-gui"
expect 'a static XD build that leaves a library out warns, naming it' \
    "--static: XD's build.sh --static built no static Qt5Gui library under \"$_bh_spec_fake_xd\"." \
    "$(_bh_spec_xd_build --static 2>&1 >/dev/null | sed -n 's/^WARNING: //p')"
expect 'XD building itself never recurses' none "$(
    XD_DIR=/r BH_ROOT=/r BH_NO_XD_BUILD=0 BUILD_DIR=$_bh_spec_xd_bd
    printf '/r/a/a.pro\n' > "$BUILD_DIR/.qmake.static"
    bh_xd_build_static
    echo none
)"

# _bh_spec_xd_shared_build [flags...]
# Parses the flags in a subshell, under the fake XD and an exported BUILD_DIR
# and BH_ROOT, sets BH_STATIC_ALL to `_bh_spec_static_all` (0 when unset), then
# runs bh_xd_build; prints what the fake build.sh recorded, or `none` when it
# did not run.
_bh_spec_xd_shared_build() (
    rm -f "$_bh_spec_fake_xd/args"
    XD_DIR=$_bh_spec_fake_xd
    export BUILD_DIR=$_bh_spec_xd_bd BH_ROOT=/r
    unset BH_STATIC BH_NO_XD_BUILD BH_TEMPLATE BH_HEADLESS BH_CROSS_SPEC
    bh_parse_args "$@"
    BH_STATIC_ALL=${_bh_spec_static_all:-0}
    bh_xd_build > /dev/null
    cat "$_bh_spec_fake_xd/args" 2>/dev/null || echo none
)
expect '--no-xd-build leaves an unbuilt XD alone' none \
    "$(_bh_spec_xd_shared_build --no-xd-build)"
expect 'a run whose every project builds static needs no shared XD' none \
    "$(_bh_spec_static_all=1; _bh_spec_xd_shared_build)"
expect 'without --static, an unbuilt XD builds, with this run'"'"'s flags' \
    '--release [||]' "$(_bh_spec_xd_shared_build --release)"
expect 'and not again, once XD has a shared build' none \
    "$(_bh_spec_xd_shared_build)"
rm "$_bh_spec_fake_xd/lib/libQt5Bootstrap.a"
expect 'a static-only module of a shared build needs its static library' \
    '--debug [||]' "$(_bh_spec_xd_shared_build --debug)"
expect 'a cross build needs the library form of its target' \
    '--cross-compile [||]' "$(_bh_spec_xd_shared_build --cross-compile)"
expect 'and not again, once XD has that form' none \
    "$(_bh_spec_xd_shared_build --cross-compile)"
expect 'XD building itself never builds XD' none "$(
    XD_DIR=/r BH_ROOT=/r BH_NO_XD_BUILD=0 BH_STATIC_ALL=0
    bh_xd_build
    echo none
)"

# bh_dependency_build runs the build.sh it is given, for any framework a
# project builds on, here a second fake one that records its arguments and
# fails while a `fail` file exists; bh_dependency_build_static does it with
# --static, once the `.qmake.static` list names a project.
_bh_spec_other=$(mktemp -d)
cat > "$_bh_spec_other/build.sh" <<'EOF'
cd "${0%/*}" || exit 1
echo "$* [${BUILD_DIR:-}|${BH_ROOT:-}]" > args
[ ! -f fail ]
EOF
# _bh_spec_dependency_build <function> [project...]
# Runs <function> on the second fake framework in a subshell, under an
# exported BUILD_DIR and BH_ROOT, the forwarded flags `--release` and
# `CONFIG+=a`, and a `.qmake.static` list that names the given projects;
# prints what the fake build.sh recorded, or `none` when it did not run.
_bh_spec_dependency_build() (
    rm -f "$_bh_spec_other/args"
    export BUILD_DIR=$_bh_spec_xd_bd BH_ROOT=/r
    _bh_spec_function=$1
    shift
    : > "$BUILD_DIR/.qmake.static"
    if [ $# -gt 0 ]; then
        printf '%s\n' "$@" > "$BUILD_DIR/.qmake.static"
    fi
    BH_FORWARD_ARGS=$(printf '%s\n' --release -- CONFIG+=a)
    "$_bh_spec_function" "$_bh_spec_other/build.sh"
    cat "$_bh_spec_other/args" 2>/dev/null || echo none
)
expect 'bh_dependency_build runs the build.sh it is given, with the flags' \
    '--release -- CONFIG+=a [|]' \
    "$(_bh_spec_dependency_build bh_dependency_build)"
expect 'bh_dependency_build_static adds --static' \
    '--static --release -- CONFIG+=a [|]' \
    "$(_bh_spec_dependency_build bh_dependency_build_static /r/a/a.pro)"
expect 'but runs nothing while no project builds static' none \
    "$(_bh_spec_dependency_build bh_dependency_build_static)"
: > "$_bh_spec_other/fail"
expect 'a failed build names its build.sh' \
    "building through \"$_bh_spec_other/build.sh\" failed (see the output above)" \
    "$(_bh_spec_dependency_build bh_dependency_build 2>&1 \
        | sed -n 's/^.*: error: //p')"
rm -rf "$_bh_spec_other"
unset -f _bh_spec_dependency_build

# bh_static_record sets BH_STATIC_ALL when its list names every `.pro` that
# --filter keeps.
_bh_spec_tree=$(mktemp -d)
mkdir -p "$_bh_spec_tree/src/a" "$_bh_spec_tree/src/b" "$_bh_spec_tree/bd"
: > "$_bh_spec_tree/src/top.pro"
: > "$_bh_spec_tree/src/a/a.pro"
: > "$_bh_spec_tree/src/b/b.pro"
# _bh_spec_static_all <filter> <static>
# Prints the BH_STATIC_ALL that bh_static_record sets under the given
# --filter and --static regexps, for the tree above.
_bh_spec_static_all() (
    BH_ROOT=$_bh_spec_tree/src BUILD_DIR=$_bh_spec_tree/bd
    rm -f "$BUILD_DIR/.qmake.static"
    BH_FILTER=$1 BH_STATIC=$2
    bh_static_record > /dev/null 2>&1
    echo "$BH_STATIC_ALL"
)
expect 'a bare --static builds every project static' 1 \
    "$(_bh_spec_static_all '' '^.*$')"
expect 'a regexp that leaves a project shared does not' 0 \
    "$(_bh_spec_static_all '' 'a/')"
expect 'nor does a run without --static' 0 "$(_bh_spec_static_all '' '')"
expect 'nor a regexp that names no project' 0 \
    "$(_bh_spec_static_all '' 'nothing')"
expect 'a regexp that names every project --filter keeps does' 1 \
    "$(_bh_spec_static_all 'a/' 'a/')"
rm -rf "$_bh_spec_tree"
unset -f _bh_spec_static_all

rm -rf "$_bh_spec_fake_xd" "$_bh_spec_xd_bd"
unset -f _bh_spec_xd_build _bh_spec_xd_shared_build _bh_spec_no_gui

# The default build dir: `-static` comes before the mode (app) or the preset
# suffix (subdirs).
# _bh_spec_build_dir <template> [flags...]
# Prints the build dir that template picks for a project at /p/ws.
_bh_spec_build_dir() (
    unset BUILD_DIR BH_STATIC BH_MODE
    BH_ROOT=/p/ws XD_DIR=/x
    _bh_spec_template=$1
    shift
    "$_bh_spec_template" /p/ws/ws.pro "$@" --get=build-dir
)
expect 'app: a shared build dir names the mode' /p/build/ws-debug \
    "$(_bh_spec_build_dir bh_template_app)"
expect 'app: --static puts -static before the mode' /p/build/ws-static-release \
    "$(_bh_spec_build_dir bh_template_app --static=x --release)"
expect 'subdirs: --static adds -static' /p/build/ws-static \
    "$(_bh_spec_build_dir bh_template_subdirs --static)"
expect 'subdirs: ahead of a preset suffix' /p/build/ws-static-debug \
    "$(BH_BUILD_DIR_SUFFIX=-debug; _bh_spec_build_dir bh_template_subdirs --static)"
unset -f _bh_spec_build_dir


test_section 'bh_filter_add -- joins a regexp to --filter from an args-parsed hook'

_bh_spec_nl='
'
expect_returns 'a regexp matches anywhere in the path' 0 \
    bh_regexp_matches 'tools/moc/moc.pro' '(moc|rcc)/'
expect_returns 'two lines: both match' 0 \
    bh_regexp_matches 'src/corelib/corelib.pro' "src/${_bh_spec_nl}corelib"
expect_returns 'two lines: one misses' 1 \
    bh_regexp_matches 'tools/corelib/corelib.pro' "src/${_bh_spec_nl}corelib"
expect_returns 'three lines: the last misses' 1 \
    bh_regexp_matches 'src/corelib/corelib.pro' "src/${_bh_spec_nl}core${_bh_spec_nl}gui"
expect_returns 'three lines: every line matches' 0 \
    bh_regexp_matches 'src/corelib/corelib.pro' "src/${_bh_spec_nl}core${_bh_spec_nl}\\.pro\$"
expect 'a message quotes a regexp' "'core'" "$(bh_regexp_text core)"
expect 'and each line of a joined one' "'src/' and 'core|gui'" \
    "$(bh_regexp_text "src/${_bh_spec_nl}core|gui")"

BH_FILTER=
bh_filter_add 'core'
expect 'without --filter, the regexp becomes the filter' core "$BH_FILTER"
bh_filter_add 'src/'
expect 'with a --filter, the regexp joins it as one more line' \
    "core${_bh_spec_nl}src/" "$BH_FILTER"
bh_filter_add 'lib'
expect 'each join adds one line' "core${_bh_spec_nl}src/${_bh_spec_nl}lib" \
    "$BH_FILTER"
bh_filter_add ''
expect 'an empty regexp changes nothing' "core${_bh_spec_nl}src/${_bh_spec_nl}lib" \
    "$BH_FILTER"
bh_filter_candidate 'src/corelib/corelib.pro'
expect 'a joined filter keeps what every line accepts' build "$BH_CANDIDATE"
bh_filter_candidate 'src/gui/gui.pro'
expect 'and skips what one line rejects' skip "$BH_CANDIDATE"
BH_FILTER=

# _bh_spec_hooked
# Adds two args-parsed hooks, the first narrowing --filter and the second
# noting the filter it sees, then parses `--filter src/`; prints what the
# second hook saw and the final filter.
_bh_spec_hooked() (
    BH_HOOKS_args_parsed=
    bh_hook args-parsed <<'}'
    bh_filter_add 'corelib'
}
    bh_hook args-parsed <<'}'
    _bh_spec_seen=$BH_FILTER
}
    BH_FILTER=
    bh_parse_args --filter 'src/'
    printf '%s|%s' "$_bh_spec_seen" "$BH_FILTER"
)
expect 'args-parsed hooks narrow the parsed --filter, in the order added' \
    "src/${_bh_spec_nl}corelib|src/${_bh_spec_nl}corelib" "$(_bh_spec_hooked)"
_bh_spec_when=added
bh_hook spec-stage <<'}'
    echo "$_bh_spec_when"
}
_bh_spec_when=run
expect 'a hook reads its variables when it runs' run "$(bh_hook_run spec-stage)"
expect 'a stage without hooks runs nothing' '' "$(bh_hook_run no-such-stage)"
expect 'a hook whose code is not valid shell stops the build when it is added' '' \
    "$(sh -c '. "$BH_TOOLS_DIR/build-handler.sh"; bh_hook spec-stage <<'"'"'}'"'"'
    if then
}
echo added' 2> /dev/null)"

# _bh_spec_hook_function
# The function a spec stage calls through bh_hook_add; prints the variable
# as it is when the stage runs.
_bh_spec_hook_function() {
    printf 'function:%s' "$_bh_spec_when"
}
_bh_spec_when=added
bh_hook_add spec-added _bh_spec_hook_function
_bh_spec_when=run
expect 'bh_hook_add calls its function when the stage runs' \
    function:run "$(bh_hook_run spec-added)"

# _bh_spec_test_env
# Adds a test-env hook, then runs a test with bh_run stubbed out; prints
# what ran, in order.
_bh_spec_test_env() (
    BH_HOOKS_test_env=
    bh_run() { printf 'ran %s|' "$2"; }
    _bh_spec_env_hook() { printf 'env|'; }
    bh_hook_add test-env _bh_spec_env_hook
    bh_run_tests /bin/true
)
expect 'bh_run_tests runs the test-env hooks before the test' \
    'env|ran /bin/true|' "$(_bh_spec_test_env)"

# _bh_spec_exit <status>
# Adds a hook to each exit stage, the last stage first, and an `on-exit` hook
# that fails, then exits <status> once bh_strict_mode ran; prints what the
# hooks saw, in the order they ran, then the status the subshell exited with.
_bh_spec_exit() {
    (
        BH_HOOKS_pre_exit= BH_HOOKS_on_exit= BH_HOOKS_post_exit=
        bh_hook post-exit <<'}'
    printf 'post:%s|' "$BH_EXIT_STATUS"
}
        bh_hook on-exit <<'}'
    printf 'on:%s|' "$BH_EXIT_STATUS"
    false
}
        bh_hook pre-exit <<'}'
    printf 'pre:%s|' "$BH_EXIT_STATUS"
}
        bh_strict_mode
        exit "$1"
    )
    printf 'exit:%s' "$?"
}
expect 'the exit stages run in order, each seeing the exit status' \
    'pre:3|on:3|post:3|exit:3' "$(_bh_spec_exit 3)"
expect 'a successful exit runs them too' 'pre:0|on:0|post:0|exit:0' \
    "$(_bh_spec_exit 0)"
unset -f _bh_spec_hooked _bh_spec_hook_function _bh_spec_test_env _bh_spec_exit
unset BH_HOOKS_spec_stage BH_HOOKS_spec_added _bh_spec_when

# Each hook's output shows, and is its result, which the next hook reads as
# BH_HOOK_RESULT; a hook still runs in the build's own shell.
bh_hook spec-result <<'}'
    _bh_spec_set=kept
    echo first
}
bh_hook spec-result <<'}'
    echo "after $BH_HOOK_RESULT"
}
_bh_spec_shown=$(bh_hook_run spec-result)
expect 'each hook'"'"'s output shows' 'first|after first' \
    "$(printf '%s\n' "$_bh_spec_shown" | tr '\n' '|' | sed 's/|$//')"
bh_hook_run spec-result > /dev/null
expect 'the last one'"'"'s result stays in BH_HOOK_RESULT' 'after first' "$BH_HOOK_RESULT"
expect 'and a variable a hook sets stays set' kept "${_bh_spec_set:-}"
unset BH_HOOKS_spec_result _bh_spec_set _bh_spec_shown


test_section 'bh_wipe -- shows a spinner during rm -rf'

# Regression guard: bh_wipe must use the background-poll spinner pattern
# (spawn, poll through bh_spin_until, render, hide), plus keep a fallback
# for when no progress display is active.

_bh_spec_wipe_body=$(awk '/^bh_wipe\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'bh_wipe runs `rm -rf "$BUILD_DIR"` held, in the background' \
    "$(printf '%s\n' "$_bh_spec_wipe_body" | grep -cE 'bh_held_start rm -rf "\$BUILD_DIR"')" 0
expect_int_gt 'bh_wipe polls through bh_spin_until' \
    "$(printf '%s\n' "$_bh_spec_wipe_body" | grep -c 'bh_spin_until')" 0
expect_int_gt 'bh_wipe renders the first spinner frame itself' \
    "$(printf '%s\n' "$_bh_spec_wipe_body" | grep -c 'bh_progress_render')" 0
expect_int_gt 'bh_wipe ends with `bh_progress_hide` (-> [√])' \
    "$(printf '%s\n' "$_bh_spec_wipe_body" | grep -c 'bh_progress_hide')" 0
expect_int_gt 'bh_wipe keeps a fallback for no progress display' \
    "$(printf '%s\n' "$_bh_spec_wipe_body" | grep -c 'if bh_progress_active')" 0
# `--wipe` is housekeeping that runs before bh_logs_init has
# initialised `_step`, so the row reads `[√]: Deleting ...`
# without a (N/M) tail. If a future change bumps `_step` here it
# would push every subsequent step's counter up by one.
expect 'bh_wipe does NOT bump `_step`' '' \
    "$(printf '%s\n' "$_bh_spec_wipe_body" | grep -E '_step=\$\(\(' | head -1)"


test_section 'bh_run_silent_fg -- counted spinner around foreground prep work'

# Regression guard for bh_run_silent_fg's counted spinner: bump `_step`,
# run cmd in foreground, ticker in background (`bh_ticker_start`), whose
# kill+wait in `bh_ticker_stop` are `|| true`-guarded, non-TTY fallback.

_bh_spec_silent_body=$(awk '/^bh_run_silent_fg\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'bh_run_silent_fg bumps `_step`' \
    "$(printf '%s\n' "$_bh_spec_silent_body" | grep -cE '_step=\$\(\(\$\{_step:-0\} \+ 1\)\)')" 0
expect_int_gt 'bh_run_silent_fg runs the command in foreground (no `&`)' \
    "$(printf '%s\n' "$_bh_spec_silent_body" | grep -cE '^[[:space:]]+"\$@"$')" 0
expect_int_gt 'bh_run_silent_fg spawns a background ticker' \
    "$(printf '%s\n' "$_bh_spec_silent_body" | grep -c 'bh_ticker_start')" 0
expect_int_gt 'bh_run_silent_fg stops the ticker' \
    "$(printf '%s\n' "$_bh_spec_silent_body" | grep -c 'bh_ticker_stop')" 0
_bh_spec_ticker_stop_body=$(awk '/^bh_ticker_stop\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")
expect_int_gt 'bh_ticker_stop `kill` is `|| true`-guarded' \
    "$(printf '%s\n' "$_bh_spec_ticker_stop_body" | grep -cE 'kill .*_bh_spinner_pid.*\|\| true')" 0
expect_int_gt 'bh_run_silent_fg keeps a fallback for no progress display' \
    "$(printf '%s\n' "$_bh_spec_silent_body" | grep -c 'if ! bh_progress_active')" 0


test_section 'bh_template_app -- BH_STEPS_TOTAL accounts for Resolving + Staging'

# Regression guard: app template's BH_STEPS_TOTAL base = 5 (3 build + 2
# silent_fg prep). --wipe is NOT counted (Deleting is housekeeping).

_bh_spec_app_body=$(awk '/^bh_template_app\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")
_bh_spec_subdirs_body=$(awk '/^bh_template_subdirs\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'bh_template_app seeds BH_STEPS_TOTAL=4 (qmake + make + Resolving + Staging)' \
    "$(printf '%s\n' "$_bh_spec_app_body" | grep -cE '^[[:space:]]+BH_STEPS_TOTAL=4')" 0
expect 'bh_template_app does NOT bump BH_STEPS_TOTAL for --wipe' '' \
    "$(printf '%s\n' "$_bh_spec_app_body" | grep -E 'BH_WIPE.*BH_STEPS_TOTAL \+' | head -1)"
expect 'bh_template_subdirs does NOT bump BH_STEPS_TOTAL for --wipe' '' \
    "$(printf '%s\n' "$_bh_spec_subdirs_body" | grep -E 'BH_WIPE.*BH_STEPS_TOTAL \+' | head -1)"


test_section 'bh_make_step -- "computing build plan" phase polls in background'

# Regression guard: "computing build plan" under `precise-blocking` (see
# `bh_progress_plan_start`) must background `bh_make_plan` and foreground-poll through bh_spin_until (`kill -0` + `bh_progress_render`
# + a `sleep` of BH_SPIN_INTERVAL).

_bh_spec_body=$(awk '/^bh_progress_plan_start\(\) \{/,/^\}$/' \
    "$_spec_dir/shell-libs/build-handler-progress.sh")

expect_int_gt 'body contains `bh_make_plan` (the precount itself)' \
    "$(printf '%s\n' "$_bh_spec_body" | grep -cE '\bbh_make_plan\b')" 0
expect_int_gt 'body runs `bh_make_plan` held, in the background (`bh_held_start`)' \
    "$(printf '%s\n' "$_bh_spec_body" | grep -cE 'bh_held_start bh_make_plan')" 0
expect_int_gt 'body polls through bh_spin_until' \
    "$(printf '%s\n' "$_bh_spec_body" | grep -c 'bh_spin_until')" 0
_bh_spec_spin_body=$(awk '/^bh_spin_until\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")
expect_int_gt 'bh_spin_until has a `while kill -0` polling loop' \
    "$(printf '%s\n' "$_bh_spec_spin_body" | grep -cE '^[[:space:]]*while kill -0')" 0
# The poll loop must call `bh_progress_tick` to advance the spinner frame
# on each pass. We can't verify it's *inside* the loop with plain grep,
# but its presence within the body is a necessary precondition -- removing
# it (or moving it outside the loop) is the exact regression we're guarding
# against.
expect_int_gt 'bh_spin_until calls bh_progress_tick' \
    "$(printf '%s\n' "$_bh_spec_spin_body" | grep -c 'bh_progress_tick')" 0

# The spinner turns once per `BH_SPIN_INTERVAL`: only `bh_progress_tick`
# advances the frame, while a redraw for a compile line keeps it.
_bh_spec_render_only_body=$(awk '/^bh_progress_render\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")
expect 'bh_progress_render keeps the spinner frame' 0 \
    "$(printf '%s\n' "$_bh_spec_render_only_body" | grep -c 'BH_PROGRESS_TICK=')"
expect 'bh_progress_tick advances the spinner frame by one' 6 \
    "$(BH_PROGRESS_TICK=5; bh_progress_tick 2>/dev/null; echo "$BH_PROGRESS_TICK")"
_bh_spec_compile_loop=$(awk '/^bh_make_step\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh" | sed -n '/^ *P)$/,/^ *;;$/p')
expect_int_gt 'a compile line redraws the row' \
    "$(printf '%s\n' "$_bh_spec_compile_loop" | grep -c 'bh_progress_render')" 0
expect 'a compile line redraws without turning the spinner' 0 \
    "$(printf '%s\n' "$_bh_spec_compile_loop" | grep -c 'bh_progress_tick')"
# And one `sleep` of BH_SPIN_INTERVAL (0.15 s, the 6.7 Hz spinner cadence).
expect_int_gt 'bh_spin_until sleeps BH_SPIN_INTERVAL to pace the poll' \
    "$(printf '%s\n' "$_bh_spec_spin_body" | grep -c 'sleep "\$BH_SPIN_INTERVAL"')" 0
expect 'BH_SPIN_INTERVAL is 0.15' 0.15 "$BH_SPIN_INTERVAL"


test_section 'bh_make -- race-aware split bumps BH_STEPS_TOTAL'

# When `bh_make` detects a Makefile with both `release-all:` and
# `debug-all:` targets it splits the make phase into two
# bh_make_step calls (make-release + make-debug). The
# `(step/total)` counter would otherwise overflow the template's
# pre-computed total (which assumes a single make), so the bump
# is part of the contract that `(3/3)` is reachable at the end of
# the second sub-step.

# Create a sandbox dir with a fake top Makefile + a no-op bh_make_step.
_bh_spec_sandbox=$(mktemp -d 2>/dev/null) || \
    _bh_spec_sandbox=/tmp/bh_spec_sandbox.$$
mkdir -p "$_bh_spec_sandbox"
cat > "$_bh_spec_sandbox/Makefile" <<'MAKEFILE'
release-all:
	@true
debug-all:
	@true
MAKEFILE

# Stub out bh_make_step so we don't actually run `make` -- the bump
# happens inside bh_make BEFORE the calls.
bh_make_step() { :; }

# `bh_make` mutates BH_STEPS_TOTAL by assignment, so we have to
# capture the post-call value through stdout -- running bh_make in
# a `(...)` subshell would let the mutation evaporate. Echo the
# final value from inside the same subshell after the call.
_bh_spec_total=$(
    BH_STEPS_TOTAL=3
    cd "$_bh_spec_sandbox"
    bh_make 1 >/dev/null 2>&1
    echo "$BH_STEPS_TOTAL"
)
expect 'bh_make bumps BH_STEPS_TOTAL by +2 on race-aware split' 5 "$_bh_spec_total"
_bh_spec_total=$(
    BH_STEPS_TOTAL=3 BH_PROGRESS_MODE=estimate
    cd "$_bh_spec_sandbox"
    bh_make 1 >/dev/null 2>&1
    echo "$BH_STEPS_TOTAL"
)
expect 'estimate hides the build-plan step of the second make run' 4 "$_bh_spec_total"

# Sandbox with NO race-aware targets -- counter should stay.
rm -f "$_bh_spec_sandbox/Makefile"
cat > "$_bh_spec_sandbox/Makefile" <<'MAKEFILE'
default:
	@true
MAKEFILE
_bh_spec_total=$(
    BH_STEPS_TOTAL=3
    cd "$_bh_spec_sandbox"
    bh_make 1 >/dev/null 2>&1
    echo "$BH_STEPS_TOTAL"
)
expect 'bh_make leaves BH_STEPS_TOTAL alone on single-make path' 3 "$_bh_spec_total"
_bh_spec_total=$(
    BH_STEPS_TOTAL=3 BH_PROGRESS_MODE=estimate
    cd "$_bh_spec_sandbox"
    bh_make 1 >/dev/null 2>&1
    echo "$BH_STEPS_TOTAL"
)
expect 'estimate leaves it alone too, as the template counts that step' 3 "$_bh_spec_total"

rm -rf "$_bh_spec_sandbox"


test_section 'bh_tee_status -- returns once its command exits, though a child lives on'

# The command starts a child that keeps the command's output open for 30 s,
# prints, and fails with 3, as a make whose compiler left a server behind; the
# step must return the 3 within seconds, with the output shown and logged.
_bh_spec_sandbox=$(mktemp -d)
_bh_spec_lingering() {
    sleep 30 &
    echo "$!" > "$_bh_spec_sandbox/child"
    echo out
    return 3
}
if bh_tail_follows_pid; then
    _bh_spec_t0=$(date +%s)
    _bh_spec_out=$(bh_tee_status "$_bh_spec_sandbox/log" _bh_spec_lingering; echo "rc=$?")
    _bh_spec_t1=$(date +%s)
    kill "$(cat "$_bh_spec_sandbox/child")" 2>/dev/null || true
    expect 'it returns the command'"'"'s exit code, after its output' \
        "out rc=3" "$(printf '%s' "$_bh_spec_out" | tr '\n' ' ')"
    expect 'and the log holds the output' out "$(cat "$_bh_spec_sandbox/log")"
    expect 'without waiting for the child' yes \
        "$([ $((_bh_spec_t1 - _bh_spec_t0)) -lt 10 ] && echo yes)"
else
    test_skip 'bh_tee_status returns once its command exits' 'tail cannot follow a pid here'
fi

# The command takes Ctrl+C: a shell that traps SIGINT and sends it to itself
# runs its trap, where one that started with the signal ignored (a command in
# the background) cannot set the trap and goes on. The trap exits normally,
# since bash takes a command substitution whose command dies of SIGINT as a
# Ctrl+C of its own. A spec run that itself started with SIGINT ignored
# passes that on to every command; hence the case runs only where a plain
# command takes the trap.
_bh_spec_self_interrupt() {
    sh -c 'trap "echo trapped; exit 3" INT; kill -INT $$; echo went on'
}
if [ "$(_bh_spec_self_interrupt)" = trapped ]; then
    _bh_spec_out=$(bh_tee_status "$_bh_spec_sandbox/log" _bh_spec_self_interrupt
        echo "rc=$?")
    expect 'Ctrl+C reaches the command' 'trapped rc=3' \
        "$(printf '%s' "$_bh_spec_out" | tr '\n' ' ')"
else
    test_skip 'Ctrl+C reaches the command' 'this run ignores SIGINT'
fi
expect 'the command reads the standard input of bh_tee_status' 'got hi' \
    "$(echo hi | bh_tee_status "$_bh_spec_sandbox/log" sh -c 'read x; echo "got $x"')"
rm -rf "$_bh_spec_sandbox"


test_section 'Ctrl+C -- a step stops at once, or ends whole first'

# Says `trapped` when Ctrl+C reaches it, `went on` when it ignores Ctrl+C,
# and succeeds either way, hence a step that runs it never fails.
_bh_spec_sandbox=$(mktemp -d)
_bh_spec_interrupt_probe() {
    sh -c 'trap "echo trapped; exit 0" INT; kill -INT $$; echo went on'
}
if [ "$(_bh_spec_self_interrupt)" = trapped ]; then
    expect 'bh_run gives Ctrl+C to its command' trapped \
        "$(LOGS=$_bh_spec_sandbox; _step=0; BH_LOG_LEVEL=1; BH_NO_PROGRESS=1
           bh_run probe _bh_spec_interrupt_probe
           cat "$_bh_spec_sandbox/01-probe.log")"
    expect 'bh_run --whole runs its command with Ctrl+C ignored' 'went on' \
        "$(LOGS=$_bh_spec_sandbox; _step=0; BH_LOG_LEVEL=1; BH_NO_PROGRESS=1
           bh_run --whole probe _bh_spec_interrupt_probe
           cat "$_bh_spec_sandbox/01-probe.log")"
    expect 'bh_run_held runs its command with Ctrl+C ignored' 'went on' \
        "$(bh_run_held _bh_spec_interrupt_probe)"

    # A shell under bh_strict_mode gets Ctrl+C while a held step runs: the
    # step still ends, then the shell stops with 130 before its next command.
    expect_returns 'Ctrl+C during a held step stops the build after it, with 130' \
        130 sh -c '. "'"$_spec_dir"'/build-handler.sh"
            bh_strict_mode
            bh_run_held sh -c "kill -INT \$1; sleep 0.3; : > \$2" \
                sh $$ "'"$_bh_spec_sandbox"'/step-ended" \
                2> "'"$_bh_spec_sandbox"'/notice"
            : > "'"$_bh_spec_sandbox"'/went-on"'
    expect 'and the held step ended' yes \
        "$([ -f "$_bh_spec_sandbox/step-ended" ] && echo yes)"
    expect 'and nothing after it ran' no \
        "$([ -f "$_bh_spec_sandbox/went-on" ] && echo yes || echo no)"
    expect 'and a notice said the build stops after the step' \
        'Ctrl+C: the build stops once this step ends, since stopping it now may leave its files half-written.' \
        "$(sed '/^$/d' "$_bh_spec_sandbox/notice")"
else
    test_skip 'Ctrl+C reaches or skips a step' 'this run ignores SIGINT'
fi

# The spinner ticker ends by itself once the shell that started it is gone,
# as after a Ctrl+C that ended the script.
sh -c '. "'"$_spec_dir"'/build-handler.sh"
       BH_SPIN_INTERVAL=0.05
       bh_ticker_start
       echo "$_bh_spinner_pid" > "'"$_bh_spec_sandbox"'/ticker"'
sleep 0.5
expect 'the ticker ends once its shell is gone' gone \
    "$(kill -0 "$(cat "$_bh_spec_sandbox/ticker")" 2>/dev/null && echo alive || echo gone)"
rm -rf "$_bh_spec_sandbox"


test_section 'bh_filter_errors -- shows the lines before each error line'

# A jom log where flex fails: the jom line names only the target, and the
# lines before it carry flex's own message. A line that goes on right after
# an error line joins its group; a gap starts a new group after `--`.
_bh_spec_sandbox=$(mktemp -d)
printf '%s\n' \
    'cl -c a.cpp' \
    'flex --noline Tokenizer.l' \
    "flex: unknown flag '-'." \
    'a.cpp' \
    'jom: Makefile.Release [release\Tokenizer.cpp] Error 1' \
    'jom: Makefile [release-all] Error 2' \
    'one' 'two' 'three' 'four' 'five' \
    'jom: Makefile [sub-angle-make_first] Error 2' \
    > "$_bh_spec_sandbox/log"
expect 'the error lines come with the lines before them, grouped' \
    "1-cl -c a.cpp|2-flex --noline Tokenizer.l|3-flex: unknown flag '-'.|4-a.cpp|5:jom: Makefile.Release [release\\Tokenizer.cpp] Error 1|6:jom: Makefile [release-all] Error 2|--|8-two|9-three|10-four|11-five|12:jom: Makefile [sub-angle-make_first] Error 2" \
    "$(bh_filter_errors "$_bh_spec_sandbox/log" 2>&1 | tr '\n' '|' | sed 's/|$//')"

# A line past 300 characters (a long compiler command) is cut there.
_bh_spec_long=$(printf '%0400d' 0)
printf '%s\n' "$_bh_spec_long" 'x.cpp(1): error C2065: y' > "$_bh_spec_sandbox/log"
expect 'a long line before an error line is cut at 300 characters' \
    "1-$(printf '%0300d' 0) ..." \
    "$(bh_filter_errors "$_bh_spec_sandbox/log" 2>&1 | head -n 1)"

# Without an error line, the log's tail shows instead.
printf '%s\n' 'all' 'fine' > "$_bh_spec_sandbox/log"
expect 'a log without an error line shows its tail' 'fine' \
    "$(bh_filter_errors "$_bh_spec_sandbox/log" 2>&1 | tail -n 1)"
rm -rf "$_bh_spec_sandbox"


test_section 'bh_strict_mode'

# bh_strict_mode should enable `set -e`. Verify in a subshell.
expect_returns 'bh_strict_mode aborts subshell on first failure' 1 sh -c '
    . "'"$_spec_dir"'/build-handler.sh"
    bh_strict_mode
    false
    echo "should-not-print"
'



# ---- specs: build modes, end to end ------------------------------------

test_section 'build modes -- --filter / --save-space / --rebuild / --clean, end to end'

# Drives bh_template_subdirs on a small project in a sandbox: a static lib,
# an app that needs it (listed before it), a debug_and_release app, two test
# apps outside the root, and an optional broken project. None of them uses
# Qt; hence --static needs no static XD libraries. Needs XD's qmake, a C++
# compiler, and make; skipped as a whole where one is missing. Every run goes
# through a subshell; hence a bh_error ends only that run.

_bh_spec_qmake=$_spec_dir/../bin/qmake
if [ ! -x "$_bh_spec_qmake" ] || ! command -v make >/dev/null 2>&1 \
        || ! command -v c++ >/dev/null 2>&1; then
    test_skip 'build modes end to end' 'needs XD qmake, make, and c++'
else
    _bh_spec_sandbox=$(mktemp -d)
    _bh_spec_src=$_bh_spec_sandbox/src
    _bh_spec_bd=$_bh_spec_sandbox/bd
    mkdir -p "$_bh_spec_src/zlib" "$_bh_spec_src/app" "$_bh_spec_src/dr" \
        "$_bh_spec_src/tests/t1" "$_bh_spec_src/tests/t2" "$_bh_spec_src/broken" \
        "$_bh_spec_src/pair"
    printf 'TEMPLATE = subdirs\nSUBDIRS = app zlib dr\napp.depends = zlib\n' \
        > "$_bh_spec_src/top.pro"
    printf 'TEMPLATE = lib\nCONFIG += staticlib\nQT =\nSOURCES = z.cpp\n' \
        > "$_bh_spec_src/zlib/zlib.pro"
    echo 'int z() { return 1; }' > "$_bh_spec_src/zlib/z.cpp"
    printf 'TEMPLATE = app\nQT =\nSOURCES = m.cpp\nLIBS += -L$$OUT_PWD/../zlib -lzlib\n' \
        > "$_bh_spec_src/app/app.pro"
    echo 'int z(); int main() { return z() - 1; }' > "$_bh_spec_src/app/m.cpp"
    printf 'TEMPLATE = app\nCONFIG += debug_and_release\nQT =\nSOURCES = m.cpp\n' \
        > "$_bh_spec_src/dr/dr.pro"
    echo 'int main() { return 0; }' > "$_bh_spec_src/dr/m.cpp"
    printf 'TEMPLATE = subdirs\nSUBDIRS = t1 t2\n' > "$_bh_spec_src/tests/tests.pro"
    for _bh_spec_t in t1 t2; do
        printf 'TEMPLATE = app\nQT =\nSOURCES = m.cpp\n' \
            > "$_bh_spec_src/tests/$_bh_spec_t/$_bh_spec_t.pro"
        cp "$_bh_spec_src/dr/m.cpp" "$_bh_spec_src/tests/$_bh_spec_t/"
    done
    printf 'error(this project is broken)\n' > "$_bh_spec_src/broken/broken.pro"
    # Two projects that share one dir, neither named after it.
    for _bh_spec_t in one two; do
        printf 'TEMPLATE = app\nQT =\nSOURCES = m.cpp\nTARGET = %s\n' "$_bh_spec_t" \
            > "$_bh_spec_src/pair/$_bh_spec_t.pro"
    done
    cp "$_bh_spec_src/dr/m.cpp" "$_bh_spec_src/pair/"
    _bh_spec_tab=$(printf '\t')

    # _bh_spec_build [flags...]
    # Runs one build of the sandbox project, headless and without progress
    # display, under the spec in `_bh_spec_qmakespec` when set. Keeps the
    # output in _bh_spec_out and the exit status in _bh_spec_rc.
    _bh_spec_xd_dir=$(cd "$_spec_dir/.." && pwd)
    _bh_spec_build() {
        _bh_spec_out=$(
            # A fresh copy of the script, since earlier sections stub parts
            # of it (bh_make_step) in this shell.
            . "$_bh_spec_xd_dir/tools/build-handler.sh"
            cd "$_bh_spec_src" || exit 1
            BH_ROOT=$_bh_spec_src
            XD_DIR=$_bh_spec_xd_dir
            BUILD_DIR=$_bh_spec_bd
            BH_SCRIPT_NAME=build.sh
            BH_HEADLESS=1 BH_NO_PROGRESS=1
            # Earlier sections leave their own settings behind (a fake
            # QMAKESPEC, remaining args, counters); start clean.
            unset BH_FILTER BH_REBUILD BH_SAVE_SPACE BH_CLEAN BH_WIPE
            unset BH_STATIC BH_NO_XD_BUILD
            unset BH_NO_BUILD BH_FORCE_QMAKE BH_QMAKE_KEEP_GOING BH_MODE
            unset BH_MODE_GIVEN BH_DEBUG_INFO
            unset BH_REMAINING_ARGS BH_TEST_ROOT BH_RUN_TESTS BH_RUN_TARGET
            unset BH_STEPS_TOTAL BH_LOG_LEVEL BH_VERBOSE BH_MEMSAFE
            unset QMAKESPEC BH_CROSS_SPEC QMAKE JOBS LOGS _step
            # The sandbox builds on the real XD, which the spec never builds.
            BH_NO_XD_BUILD=1
            QMAKESPEC=${_bh_spec_qmakespec:-}
            bh_template_subdirs "$_bh_spec_src/top.pro" "$@" </dev/null 2>&1
        )
        _bh_spec_rc=$?
    }
    # _bh_spec_built
    # Prints the projects the last build built, in order, space-separated.
    _bh_spec_built() {
        printf '%s\n' "$_bh_spec_out" \
            | sed -n 's/^Filtered build\( \[[0-9/]*\]\)\{0,1\}: //p; s/^--save-space build \[[0-9/]*\]: //p' \
            | tr '\n' ' '
    }
    # _bh_spec_lines <order|finished|unfinished>
    # Prints the projects of that kind of cache line, in order.
    _bh_spec_lines() {
        awk -F'\t' -v kind="$1" '
            (kind == "order"      && NF == 3) ||
            (kind == "finished"   && NF >= 4 && $4 != "") ||
            (kind == "unfinished" && NF >= 4 && $4 == "") {
                sub(/^.*\/src\//, "", $1); printf "%s ", $1
            }' "$_bh_spec_bd/.qmake.save_space" 2>/dev/null
    }

    _bh_spec_build
    expect 'plain build succeeds' 0 "$_bh_spec_rc"
    expect 'plain build writes no cache' '' \
        "$(ls "$_bh_spec_bd/.qmake.save_space" 2>/dev/null)"

    _bh_spec_build --wipe --no-build
    expect '--wipe removes the build dir' '' "$(ls -d "$_bh_spec_bd" 2>/dev/null)"

    _bh_spec_build --filter 'zlib|app|dr|tests/t1'
    expect '--filter on a fresh dir succeeds' 0 "$_bh_spec_rc"
    expect '--filter records the order first' 1 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c '^Recording build order')"
    expect '--filter builds in dependency order, then the unrecorded match' \
        'zlib/zlib.pro app/app.pro dr/dr.pro tests/t1/t1.pro ' "$(_bh_spec_built)"
    expect 'the cache lists one order line per variant' \
        'zlib/zlib.pro app/app.pro dr/dr.pro dr/dr.pro ' "$(_bh_spec_lines order)"
    expect 'without --save-space no finished line is written' '' \
        "$(_bh_spec_lines finished)"

    _bh_spec_build --filter 'zlib|app'
    expect '--filter again reuses the order (no qmake run)' 0 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c '^Recording build order')"
    expect '--filter again builds only its matches' \
        'zlib/zlib.pro app/app.pro ' "$(_bh_spec_built)"

    _bh_spec_build --filter 'tests/t2'
    expect 'an unrecorded single match builds alone' 'tests/t2/t2.pro ' \
        "$(_bh_spec_built)"

    _bh_spec_build --filter 'tests/tests.pro'
    expect 'a subdirs .pro the filter names builds too' 'tests/tests.pro ' \
        "$(_bh_spec_built)"

    _bh_spec_build --filter 'pair/'
    expect 'two projects of one dir build alone' 'pair/one.pro pair/two.pro ' \
        "$(_bh_spec_built)"
    expect 'each from the Makefile named after its .pro' \
        'Makefile.one Makefile.two ' \
        "$(cd "$_bh_spec_bd/pair" && ls Makefile* | tr '\n' ' ')"
    : > "$_bh_spec_sandbox/pair-built"
    _bh_spec_build --filter 'pair/'
    expect 'built again, they reuse those Makefiles (no qmake run)' '' \
        "$(find "$_bh_spec_bd/pair" -name 'Makefile*' -newer "$_bh_spec_sandbox/pair-built")"
    expect 'and still build' 'pair/one.pro pair/two.pro ' "$(_bh_spec_built)"

    _bh_spec_build --filter 'nothing-here'
    expect 'a filter that matches nothing fails' 1 "$_bh_spec_rc"

    _bh_spec_build --qmake --filter 'app'
    expect '--qmake with --filter records again, then filters' 1 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c '^Recording build order')"
    expect '--qmake keeps the order lines' \
        'zlib/zlib.pro app/app.pro dr/dr.pro dr/dr.pro ' "$(_bh_spec_lines order)"

    _bh_spec_build --filter 'zlib|app' --rebuild=app
    expect 'without --save-space, --rebuild builds every match, in order' \
        'zlib/zlib.pro app/app.pro ' "$(_bh_spec_built)"
    expect 'and cleans the ones the sub-filter names first' \
        '--rebuild: app/app.pro matches; building it again.' \
        "$(printf '%s\n' "$_bh_spec_out" | grep '^--rebuild:')"
    expect 'and it still writes no finished line' '' "$(_bh_spec_lines finished)"
    _bh_spec_build --rebuild
    expect 'a bare --rebuild alone rebuilds the whole root in order' \
        'zlib/zlib.pro app/app.pro dr/dr.pro ' "$(_bh_spec_built)"
    expect 'cleaning each project first' 3 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c '^--rebuild:')"
    _bh_spec_build --rebuild=tests/t2
    expect '--rebuild alone builds the root, then reaches its unrecorded match' \
        'zlib/zlib.pro app/app.pro dr/dr.pro tests/t2/t2.pro ' "$(_bh_spec_built)"
    expect 'cleaning only that match' 1 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c '^--rebuild:')"

    rm -rf "$_bh_spec_bd"
    _bh_spec_build --save-space
    expect '--save-space on a fresh dir succeeds' 0 "$_bh_spec_rc"
    expect '--save-space builds the whole root in order' \
        'zlib/zlib.pro app/app.pro dr/dr.pro ' "$(_bh_spec_built)"
    expect '--save-space writes a finished line per variant' \
        'zlib/zlib.pro app/app.pro dr/dr.pro dr/dr.pro ' "$(_bh_spec_lines finished)"
    expect '--save-space leaves no object file' 0 \
        "$(find "$_bh_spec_bd" -name '*.o' | wc -l | tr -d ' ')"

    _bh_spec_build --save-space
    expect '--save-space again skips every finished project' '' "$(_bh_spec_built)"
    expect '--save-space again reports the skips' 1 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c 'built 0, skipped 3 up-to-date, of 3')"

    _bh_spec_build --save-space --rebuild
    expect 'bare --rebuild builds every project again' \
        'zlib/zlib.pro app/app.pro dr/dr.pro ' "$(_bh_spec_built)"
    expect '--rebuild leaves no emptied finished line behind' '' \
        "$(_bh_spec_lines unfinished)"

    _bh_spec_build --save-space --rebuild=zlib
    expect '--rebuild=<regexp> builds only its matches again' 'zlib/zlib.pro ' \
        "$(_bh_spec_built)"

    _bh_spec_build --save-space --filter 'zlib|app' --rebuild=app
    expect '--rebuild narrows on top of --filter' 'app/app.pro ' "$(_bh_spec_built)"

    _bh_spec_build --save-space --filter 'zlib' --rebuild=app
    expect '--rebuild cannot reach past --filter' '' "$(_bh_spec_built)"

    _bh_spec_build --save-space --filter 'zlib|tests/t1'
    expect 'an unrecorded match builds alone under --save-space' 'tests/t1/t1.pro ' \
        "$(_bh_spec_built)"
    _bh_spec_build --save-space --filter 'zlib|tests/t1'
    expect 'and counts as recorded and finished from then on' '' "$(_bh_spec_built)"
    _bh_spec_build --save-space --filter 'tests/t1' --rebuild
    expect 'until --rebuild names it' 'tests/t1/t1.pro ' "$(_bh_spec_built)"

    _bh_spec_build --save-space --filter 'tests/'
    expect 'under --save-space a matched subdirs .pro is passed over' 0 "$_bh_spec_rc"
    expect 'while its unfinished leaves build alone' 'tests/t2/t2.pro ' \
        "$(_bh_spec_built)"
    expect 'saying why the subdirs .pro itself did not build' 1 \
        "$(printf '%s\n' "$_bh_spec_out" \
            | grep -c '^--save-space: tests/tests.pro is a subdirs project')"
    _bh_spec_build --save-space --filter 'tests/'
    expect 'and nothing builds again after that' '' "$(_bh_spec_built)"

    _bh_spec_build --save-space --qmake
    expect '--qmake keeps every finished line' \
        'app/app.pro dr/dr.pro dr/dr.pro tests/t1/t1.pro tests/t2/t2.pro zlib/zlib.pro ' \
        "$(_bh_spec_lines finished | tr ' ' '\n' | sort | tr '\n' ' ')"

    _bh_spec_build --save-space --clean --filter 'app'
    expect 'a filtered --clean forgets only its matches' 'app/app.pro ' \
        "$(_bh_spec_built)"
    _bh_spec_build --save-space --clean --filter 'zlib|dr' --rebuild=dr
    expect 'a filtered --clean honours --rebuild as the sub-filter' 'dr/dr.pro ' \
        "$(_bh_spec_built)"

    rm -f "$_bh_spec_bd/zlib/libzlib.a"
    _bh_spec_build --save-space --filter 'zlib'
    expect 'a finished project whose target went missing builds again' \
        'zlib/zlib.pro ' "$(_bh_spec_built)"

    # An explicit mode builds the tree in that one variant, and a switch to
    # the other one builds every project again, as both variants
    # write to the same files.
    rm -rf "$_bh_spec_bd"
    _bh_spec_build --save-space --debug
    expect '--debug builds the whole root' \
        'zlib/zlib.pro app/app.pro dr/dr.pro ' "$(_bh_spec_built)"
    expect 'as debug only' 0 \
        "$(grep -c -- '-DQT_NO_DEBUG' "$_bh_spec_bd/zlib/Makefile")"
    _bh_spec_build --save-space --debug
    expect '--debug again skips every finished project' '' "$(_bh_spec_built)"
    _bh_spec_build --save-space --release
    expect '--release over it builds every project again' \
        'zlib/zlib.pro app/app.pro dr/dr.pro ' "$(_bh_spec_built)"
    expect 'as release only' 1 \
        "$(grep -c -- '-DQT_NO_DEBUG' "$_bh_spec_bd/zlib/Makefile")"
    _bh_spec_build --filter 'zlib' --debug
    expect 'without --save-space, a switch makes the Makefile again' 0 \
        "$(grep -c -- '-DQT_NO_DEBUG' "$_bh_spec_bd/zlib/Makefile")"
    expect 'cleaning what the other variant built first' 1 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c 'made for another toolchain or variant')"

    _bh_spec_build --clean --no-build
    expect 'a bare --clean removes the cache' '' \
        "$(ls "$_bh_spec_bd/.qmake.save_space" 2>/dev/null)"

    # --clean scoped by the filters, on a plain build (which keeps objects).
    # _bh_spec_objects <dir>: the object files under that build subdir.
    _bh_spec_objects() {
        find "$_bh_spec_bd/$1" -name '*.o' 2>/dev/null | wc -l | tr -d ' '
    }
    rm -rf "$_bh_spec_bd"
    _bh_spec_build --filter 'zlib|app|dr|tests/t1'
    expect 'a plain build keeps its objects' 1 "$(_bh_spec_objects zlib)"
    _bh_spec_build --clean --no-build --filter 'app'
    expect '--clean with --filter cleans only the matches (app)' 0 \
        "$(_bh_spec_objects app)"
    expect 'and leaves the rest (zlib, dr, t1) alone' \
        "1 $(_bh_spec_objects dr) 1" \
        "$(_bh_spec_objects zlib) $(_bh_spec_objects dr) $(_bh_spec_objects tests/t1)"
    _bh_spec_build --clean --no-build --filter 'zlib|dr' --rebuild=dr
    expect '--clean with both filters cleans only what both name (dr)' 0 \
        "$(_bh_spec_objects dr)"
    expect 'and leaves the base-filter-only match (zlib) alone' 1 \
        "$(_bh_spec_objects zlib)"
    _bh_spec_build --clean --no-build --rebuild=tests/t1
    expect '--clean with --rebuild alone cleans its match (t1)' 0 \
        "$(_bh_spec_objects tests/t1)"
    expect 'and leaves zlib alone' 1 "$(_bh_spec_objects zlib)"
    _bh_spec_build --clean --no-build
    expect 'a bare --clean cleans the whole root' 0 "$(_bh_spec_objects zlib)"
    unset -f _bh_spec_objects

    # Switching between the two modes on one build dir.
    rm -rf "$_bh_spec_bd"
    _bh_spec_build --filter 'zlib|app|dr|tests/t1'
    _bh_spec_build --save-space --filter 'zlib|app|dr|tests/t1'
    expect 'a project built without --save-space gets marked finished by it' \
        'zlib/zlib.pro app/app.pro dr/dr.pro dr/dr.pro tests/t1/t1.pro ' \
        "$(_bh_spec_lines finished)"
    _bh_spec_build --save-space --filter 'zlib|app|dr|tests/t1'
    expect 'and is skipped from then on' '' "$(_bh_spec_built)"
    _bh_spec_build --filter 'zlib|app|dr|tests/t1'
    expect 'back without --save-space, the Makefiles lose its hooks' 0 \
        "$(grep -c '^save_space_finished:' "$_bh_spec_bd/app/Makefile" \
            "$_bh_spec_bd/tests/t1/Makefile" | awk -F: '{ n += $2 } END { print n + 0 }')"

    # --static: projects that switch to and from `qt_static` mode, through a
    # filtered build (each project qmaked alone) and a plain one (one
    # recursive qmake of the root).
    # _bh_spec_static <dir>: 1 when the Makefiles of that build subdir compile
    # in `qt_static` mode, else 0.
    _bh_spec_static() {
        if cat "$_bh_spec_bd/$1"/Makefile* 2>/dev/null \
                | grep -q '^DEFINES .*-DQT_STATIC'; then
            echo 1
        else
            echo 0
        fi
    }
    rm -rf "$_bh_spec_bd"
    _bh_spec_build --filter 'zlib|app' --static=zlib --no-xd-build
    expect '--static=<regexp> with --filter succeeds' 0 "$_bh_spec_rc"
    expect 'and lists only its match' "$_bh_spec_src/zlib/zlib.pro" \
        "$(cat "$_bh_spec_bd/.qmake.static")"
    expect 'a fresh build dir has no switch to report' '' \
        "$(printf '%s\n' "$_bh_spec_out" | grep '^--static:')"
    expect 'its match is qmaked in qt_static mode, the rest shared' '1 0' \
        "$(_bh_spec_static zlib) $(_bh_spec_static app)"
    rm -rf "$_bh_spec_bd"
    _bh_spec_build --filter 'zlib|app'
    expect 'a build without --static makes no static list' '' \
        "$(ls "$_bh_spec_bd/.qmake.static" 2>/dev/null)"
    _bh_spec_build --filter 'zlib|app' --static=zlib --no-xd-build
    expect 'a match built shared before reports its switch' \
        '--static: zlib/zlib.pro switches to qt_static mode; cleaning zlib/Makefile.' \
        "$(printf '%s\n' "$_bh_spec_out" | grep '^--static:')"
    expect 'the match is qmaked again in qt_static mode, the rest shared' '1 0' \
        "$(_bh_spec_static zlib) $(_bh_spec_static app)"
    _bh_spec_build --filter 'zlib|app' --static=zlib --no-xd-build
    expect 'the same --static again switches nothing' '' \
        "$(printf '%s\n' "$_bh_spec_out" | grep '^--static:')"
    _bh_spec_build --filter 'zlib|app'
    expect 'without --static the match switches back to shared' \
        '--static: zlib/zlib.pro switches to shared mode; cleaning zlib/Makefile.' \
        "$(printf '%s\n' "$_bh_spec_out" | grep '^--static:')"
    expect 'and is qmaked again shared' '0 0' \
        "$(_bh_spec_static zlib) $(_bh_spec_static app)"
    expect 'leaving an empty list behind' '' "$(cat "$_bh_spec_bd/.qmake.static")"
    _bh_spec_build --static='app|dr' --no-xd-build
    expect 'a plain build with --static=<regexp> succeeds' 0 "$_bh_spec_rc"
    expect 'its recursive qmake applies the regexp per project' '0 1 1' \
        "$(_bh_spec_static zlib) $(_bh_spec_static app) $(_bh_spec_static dr)"
    _bh_spec_build --static --filter 'zlib' --no-xd-build
    expect '--static narrows on --filter like --rebuild (zlib only)' \
        "$_bh_spec_src/zlib/zlib.pro" "$(cat "$_bh_spec_bd/.qmake.static")"
    _bh_spec_build --no-static --no-xd-build
    expect '--no-static builds every project shared again' '0 0 0' \
        "$(_bh_spec_static zlib) $(_bh_spec_static app) $(_bh_spec_static dr)"
    unset -f _bh_spec_static

    # A build-deps hook runs when the run builds, before any qmake, and not
    # under --no-build.
    bh_hook build-deps <<'}'
    echo "build-deps hook ran"
}
    _bh_spec_build --qmake --filter 'zlib'
    expect 'a build-deps hook runs when the run builds' 1 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c '^build-deps hook ran$')"
    expect 'before any qmake runs' 'build-deps hook ran' \
        "$(printf '%s\n' "$_bh_spec_out" \
            | grep -m1 -E '^build-deps hook ran$|^Recording build order')"
    _bh_spec_build --no-build
    expect 'but not under --no-build' 0 \
        "$(printf '%s\n' "$_bh_spec_out" | grep -c '^build-deps hook ran$')"
    unset BH_HOOKS_build_deps

    # A build dir switched to another toolchain is cleaned before qmake runs
    # again; hence no object of the old toolchain reaches the new link.
    if [ "$(uname -s)" = Linux ] && command -v clang++ >/dev/null 2>&1 \
            && command -v readelf >/dev/null 2>&1; then
        # _bh_spec_compiler <dir>: the compiler the `.comment` section of
        # that build subdir's object names, `GCC` or `clang`.
        _bh_spec_compiler() {
            readelf -p .comment "$_bh_spec_bd/$1"/*.o 2>/dev/null \
                | grep -oE 'clang|GCC' | head -1
        }
        rm -rf "$_bh_spec_bd"
        _bh_spec_build --filter 'zlib|app'
        expect 'the default spec compiles with GCC' 'GCC GCC' \
            "$(_bh_spec_compiler zlib) $(_bh_spec_compiler app)"
        _bh_spec_qmakespec=linux-clang
        _bh_spec_build --filter 'zlib|app'
        unset _bh_spec_qmakespec
        expect 'a build with another spec succeeds' 0 "$_bh_spec_rc"
        expect 'after cleaning what the old toolchain built, project by project' 2 \
            "$(printf '%s\n' "$_bh_spec_out" | grep -c 'made for another toolchain')"
        expect 'and compiles everything again with the new one' 'clang clang' \
            "$(_bh_spec_compiler zlib) $(_bh_spec_compiler app)"
        unset -f _bh_spec_compiler
    else
        test_skip 'a toolchain switch cleans the old objects' 'needs Linux, clang++, and readelf'
    fi

    # A broken project the filter leaves out.
    printf 'TEMPLATE = subdirs\nSUBDIRS = app broken zlib dr\napp.depends = zlib\n' \
        > "$_bh_spec_src/top.pro"
    rm -rf "$_bh_spec_bd"
    _bh_spec_build --filter 'zlib|app'
    expect 'a broken unrelated project only warns under --filter' 0 "$_bh_spec_rc"
    expect 'and the recording stops at it (app pulls zlib in first)' \
        'zlib/zlib.pro app/app.pro ' "$(_bh_spec_lines order)"
    expect 'while every match still builds' 'zlib/zlib.pro app/app.pro ' \
        "$(_bh_spec_built)"
    _bh_spec_build --force-qmake --filter 'zlib|app'
    expect '--force-qmake records past the broken project' \
        'zlib/zlib.pro app/app.pro dr/dr.pro dr/dr.pro ' "$(_bh_spec_lines order)"
    _bh_spec_build --save-space
    expect 'a broken project fails a --save-space build' 2 "$_bh_spec_rc"

    rm -rf "$_bh_spec_sandbox"
    unset -f _bh_spec_build _bh_spec_built _bh_spec_lines
fi


test_end

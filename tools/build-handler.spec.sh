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
. "$_spec_dir/build-handler.sh"
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
    "$(printf '%s\n' "$_bh_spec_render_body" | grep -cF "printf ': %s' \"\$BH_PROGRESS_DESC\"")" 1
expect_int_gt 'bar DONE moves cursor up over the header row (\\033[1A)' \
    "$(printf '%s\n' "$_bh_spec_render_body" | grep -cF '\033[1A')" 0
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
    "$(printf '%s\n' "$_bh_spec_runqmake_body" | grep -c 'bh_newer_project_file Makefile')" 0
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

rm -rf "$_bh_spec_sandbox"
BH_ROOT=$_bh_spec_root
BUILD_DIR=$_bh_spec_build_dir
unset BH_FILTER BH_REBUILD BH_CANDIDATE


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

# Regression guard: "computing build plan" must background `bh_make_run -n`
# and foreground-poll through bh_spin_until (`kill -0` + `bh_progress_render`
# + a `sleep` of BH_SPIN_INTERVAL).

_bh_spec_body=$(awk '/^bh_make_step\(\) \{/,/^\}$/' \
    "$_spec_dir/build-handler.sh")

expect_int_gt 'body contains `bh_make_run -n` (the precount itself)' \
    "$(printf '%s\n' "$_bh_spec_body" | grep -cE '\bbh_make_run -n\b')" 0
expect_int_gt 'body runs `bh_make_run -n` held, in the background (`bh_held_start`)' \
    "$(printf '%s\n' "$_bh_spec_body" | grep -cE 'bh_held_start bh_make_run -n')" 0
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
# apps outside the root, and an optional broken project. Needs XD's qmake, a
# C++ compiler, and make; skipped as a whole where one is missing. Every run
# goes through a subshell, so a bh_error ends only that run.

_bh_spec_qmake=$_spec_dir/../bin/qmake
if [ ! -x "$_bh_spec_qmake" ] || ! command -v make >/dev/null 2>&1 \
        || ! command -v c++ >/dev/null 2>&1; then
    test_skip 'build modes end to end' 'needs XD qmake, make, and c++'
else
    _bh_spec_sandbox=$(mktemp -d)
    _bh_spec_src=$_bh_spec_sandbox/src
    _bh_spec_bd=$_bh_spec_sandbox/bd
    mkdir -p "$_bh_spec_src/zlib" "$_bh_spec_src/app" "$_bh_spec_src/dr" \
        "$_bh_spec_src/tests/t1" "$_bh_spec_src/tests/t2" "$_bh_spec_src/broken"
    printf 'TEMPLATE = subdirs\nSUBDIRS = app zlib dr\napp.depends = zlib\n' \
        > "$_bh_spec_src/top.pro"
    printf 'TEMPLATE = lib\nCONFIG += staticlib\nSOURCES = z.cpp\n' \
        > "$_bh_spec_src/zlib/zlib.pro"
    echo 'int z() { return 1; }' > "$_bh_spec_src/zlib/z.cpp"
    printf 'TEMPLATE = app\nSOURCES = m.cpp\nLIBS += -L$$OUT_PWD/../zlib -lzlib\n' \
        > "$_bh_spec_src/app/app.pro"
    echo 'int z(); int main() { return z() - 1; }' > "$_bh_spec_src/app/m.cpp"
    printf 'TEMPLATE = app\nCONFIG += debug_and_release\nSOURCES = m.cpp\n' \
        > "$_bh_spec_src/dr/dr.pro"
    echo 'int main() { return 0; }' > "$_bh_spec_src/dr/m.cpp"
    printf 'TEMPLATE = subdirs\nSUBDIRS = t1 t2\n' > "$_bh_spec_src/tests/tests.pro"
    for _bh_spec_t in t1 t2; do
        printf 'TEMPLATE = app\nSOURCES = m.cpp\n' \
            > "$_bh_spec_src/tests/$_bh_spec_t/$_bh_spec_t.pro"
        cp "$_bh_spec_src/dr/m.cpp" "$_bh_spec_src/tests/$_bh_spec_t/"
    done
    printf 'error(this project is broken)\n' > "$_bh_spec_src/broken/broken.pro"
    _bh_spec_tab=$(printf '\t')

    # _bh_spec_build [flags...]
    # Runs one build of the sandbox project, headless and without progress
    # display. Keeps the output in _bh_spec_out and the exit status in
    # _bh_spec_rc.
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
            unset BH_NO_BUILD BH_FORCE_QMAKE BH_QMAKE_KEEP_GOING BH_MODE
            unset BH_REMAINING_ARGS BH_TEST_ROOT BH_RUN_TESTS BH_RUN_TARGET
            unset BH_STEPS_TOTAL BH_LOG_LEVEL BH_VERBOSE BH_MEMSAFE
            unset QMAKESPEC BH_CROSS_SPEC QMAKE JOBS LOGS _step
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

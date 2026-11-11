# License: Apache 2.0 without attribution need.
#
# Build order and filters of `build-handler.sh`, which sources this file after
# its own helpers: the `.qmake.save_space` cache that save_order.prf and
# save_space.prf fill, the walk of its order lines, and --filter, --rebuild,
# and --save-space on top of that order. build-handler.md describes them.


# bh_pro_files
#
# Prints each `.pro` under the source root, sorted, leaving out the build
# folders, `.git`, and the scratch folder `todo/`.
bh_pro_files() {
    { find -H "$BH_ROOT" -name '*.pro' ! -path '*/build/*' ! -path '*/.git/*' \
        ! -path "$BH_ROOT/todo/*" 2>/dev/null || true; } | sort
}

# Args: [--relax]
bh_filter_paths() {
    if [ "${1:-}" = --relax ]; then
        awk -v root="$BH_ROOT/" '{ print substr($0, length(root) + 1) }'
    else
        awk -v root="$BH_ROOT/" 'index($0, root) == 1 { print substr($0, length(root) + 1) }'
    fi \
    | bh_grep_lines "${BH_FILTER:-${BH_REBUILD:-}}" \
    | awk -v root="$BH_ROOT/" '{ print root $0 }'
}

# Args: <regexp>
bh_grep_lines() {
    _bgl_line=${1%%"
"*}
    if [ "$_bgl_line" = "$1" ]; then
        grep -E -- "$_bgl_line" || [ $? -eq 1 ]
        return
    fi
    { grep -E -- "$_bgl_line" || [ $? -eq 1 ]; } | bh_grep_lines "${1#*"
"}"
}

# bh_build_in_order <pro>
#
# The build path of --save-space and of --filter. It drives the build from the
# one `.qmake.save_space` cache at the build root, which save_order.prf and
# save_space.prf (include()d by default_post.prf) fill with two kinds of
# tab-separated lines: the three-column "<pro>\t<variant>\t<target>" written
# once for every leaf as qmake generates it, and, under --save-space only, the
# same line plus a fourth column, the time, appended at post-link once that
# variant linked. A finished line whose time is empty counts as not finished.
# An order line stays in the cache from run to run, and is never written
# twice. This script only reads the cache, empties a finished line's time
# (--rebuild) and drops that emptied line once post-link appended a fresh one,
# or drops finished lines (--clean), all only for the projects its filters
# name; it never appends and never moves a line:
#
#   0. Without --save-space and --filter, a --rebuild still comes here, so
#      its matches build again in the recorded order.
#   1. Record the build order when needed (see bh_build_order_record): one
#      qmake run over <pro>, the entire project, lists the whole tree in build
#      order, dependencies first. No earlier build is needed. With --filter
#      and/or --rebuild, qmake then the parts of the tree that this recording
#      did not reach but the filters name (see bh_build_order_extend).
#   2. Walk the cache in that order (see bh_build_walk) and `make` each project
#      that --filter keeps, in its shadow build dir. Under --save-space, a
#      project with a finished line for each of its variants (and whose
#      recorded targets still exist) is skipped, which is what lets a killed
#      build resume without repeating finished projects. A project missing one
#      failed, or was killed, before its post-link ran, so it is built again;
#      only that project, never the whole tree. Dates play no part: make
#      decides what a build compiles, and --rebuild[=<regexp>] is how a
#      finished project is built again on purpose (its finished lines get an
#      empty time first, and post-link appends a fresh one once it links).
#   3. With --filter and/or --rebuild, go on with the projects that have no
#      order line but that a subdirs project lists, or that have none above
#      them (see bh_build_unrecorded); --filter filters what qmake lists, hence
#      a project that no subdirs project lists on this platform is left out.
bh_build_in_order() {
    BH_ORDER_ROOT=$1
    # Without --filter, only a --rebuild that names its matches reaches the
    # projects the cache does not list; the root project covers the rest.
    _bio_unrecorded=1
    if [ -z "${BH_FILTER:-}" ] \
            && { [ -z "${BH_REBUILD:-}" ] || [ "$BH_REBUILD" = '^.*$' ]; }; then
        _bio_unrecorded=0
    fi
    bh_run --whole build-order bh_prepare_build_order "$1" "$_bio_unrecorded"
    bh_build_walk prepare
    _bw_count=$_bw_total
    # Counts those projects too, hence the counter of each line covers both.
    if [ "$_bio_unrecorded" -eq 1 ]; then
        _bw_count=$((_bw_count + $(bh_build_unrecorded count)))
    fi
    bh_build_walk build
    if [ "${BH_SAVE_SPACE:-0}" -eq 1 ]; then
        echo "--save-space: built $_bw_made, skipped $_bw_skipped up-to-date, of $_bw_total project(s)."
    fi
    [ "$_bio_unrecorded" -eq 1 ] || return 0
    _bio_status=0
    bh_build_unrecorded || _bio_status=$?
    case "$_bio_status" in
        0) ;;
        10)
            bh_filter_has_projects || bh_error \
                "--filter: no project of the build under '$BH_ROOT' matches $(bh_regexp_text "$BH_FILTER")"
            ;;
        *) exit "$_bio_status" ;;
    esac
}

bh_prepare_build_order() {
    bh_build_order_record "$1"
    if [ "$2" -eq 1 ]; then
        bh_build_order_extend
    fi
}

# bh_build_order_record <pro>
#
# Records the build order into the `.qmake.save_space` cache at the build
# root, creating the cache when it is missing. It qmakes the entire project
# the way a build does: a plain qmake of <pro> (no `-r`), then `make
# qmake_all`, which runs a plain qmake for each subdir as it reaches it,
# dependencies first. Under CONFIG save_order, save_order.prf then writes one
# order line per variant of every leaf project, in build order.
#
# Under --save-space that qmake runs on every run, as it is also what adds
# the post-link steps. Else it only runs when the cache holds no order line,
# or when --qmake/--refresh asks; otherwise the recorded order is reused, and
# make itself runs qmake again for a project whose Makefile is out of date.
# The lines of earlier runs stay, so a run only adds the order lines the
# cache lacks.
#
# Without --save-space, a recording that fails (say, a broken project that the
# filter leaves out) only warns: the projects it reached keep their recorded
# order, and bh_build_unrecorded builds the rest.
bh_build_order_record() {
    _bor_pro=$1
    # Under --save-space, a build root made for another toolchain or variant
    # starts its cache anew, and qmake records the order again below: both
    # builds write to the same files (XD's `lib/` on Linux, for one), hence a
    # finished line names what the other build made, and an order line the
    # other variant's target. Without --save-space no line counts as finished;
    # each project the filters name is cleaned on its own instead (see
    # `bh_build_candidate`).
    if [ "${BH_SAVE_SPACE:-0}" -eq 1 ] && [ -f "$BUILD_DIR/Makefile" ] \
            && bh_makefile_build_differs "$BUILD_DIR/Makefile"; then
        : > "$BUILD_DIR/.qmake.save_space"
    fi
    if [ "${BH_SAVE_SPACE:-0}" -ne 1 ] && [ "${BH_FORCE_QMAKE:-0}" -eq 0 ] \
            && bh_save_space_has_line 'NF == 3'; then
        return 0
    fi
    if [ ! -f "$BUILD_DIR/.qmake.save_space" ]; then
        : > "$BUILD_DIR/.qmake.save_space"
    fi
    # Drop stale EMPTY .qmake.cache files first. A killed build or a
    # force_bootstrap sub-build can leave a 0-byte .qmake.cache in a subdir; a
    # fresh top-level qmake then treats that subdir as a project root and
    # searches its mkspecs before .qmake.conf, missing MODULE_VERSION and dying
    # with "Module does not define version." They hold no config (0 bytes), so
    # removing them is safe -- qmake recreates the real root marker as needed.
    find -H "$BUILD_DIR" -name .qmake.cache -size 0 -delete 2>/dev/null || :
    echo "Recording build order via qmake..."
    if [ "${BH_SAVE_SPACE:-0}" -eq 1 ]; then
        bh_qmake_in_build_order "$_bor_pro"
        return 0
    fi
    # In a subshell, since a failed step exits its shell.
    _bor_status=0
    ( bh_qmake_in_build_order "$_bor_pro" CONFIG+=save_order ) \
        || _bor_status=$?
    if [ "$_bor_status" -ne 0 ]; then
        _bor_hint="Pass --force-qmake to record the projects after the failed one too."
        if [ "${BH_QMAKE_KEEP_GOING:-0}" -ne 0 ]; then
            _bor_hint="Every project but the failed one(s) is recorded."
        fi
        bh_warning "Recording the build order failed (exit $_bor_status)." \
            "The projects it reached build in their recorded order, the rest after them." \
            "$_bor_hint"
    fi
}

# bh_qmake_in_build_order <pro> [extra-qmake-args...]
#
# Qmakes the entire project of <pro> in the cwd, each subdir in the order a
# build reaches it: a plain qmake of <pro>, then `make qmake_all`, which stops
# at the first subdir that fails. Under --force-qmake it is `make -k
# qmake_all`, which goes on with the other subdirs, and still reports failure.
bh_qmake_in_build_order() {
    _bqo_pro=$1; shift
    # Forced, since bh_run_qmake normally SKIPS qmake when the Makefile is
    # already up-to-date, while only a qmake that runs writes the order lines.
    _bqo_saved_fq=${BH_FORCE_QMAKE:-0}
    BH_FORCE_QMAKE=1
    BH_QMAKE_PLAIN=1
    # shellcheck disable=SC2086  # intentional word-split of forwarded args.
    bh_run_qmake qmake "$_bqo_pro" "$@" $BH_REMAINING_ARGS
    BH_FORCE_QMAKE=$_bqo_saved_fq
    BH_QMAKE_PLAIN=0
    if [ "${BH_QMAKE_KEEP_GOING:-0}" -ne 0 ]; then
        bh_run --whole qmake-all bh_make_run -k qmake_all
    else
        bh_run --whole qmake-all bh_make_run qmake_all
    fi
}

# bh_filter_candidate <path>
#
# Sorts one build candidate, named by its path relative to BH_ROOT, into
# BH_CANDIDATE, for every build loop to share: `skip` when --filter's regexp
# rejects the path, else `rebuild` when --rebuild's regexp accepts it on top of
# that, else `build`. Next to it, BH_CANDIDATE_STATIC is 1 when the path is
# not skipped and --static's regexp accepts it, else 0. A regexp that was not
# given accepts every path for --filter, and none for --rebuild and --static.
# All are extended regexps (`grep -E`), matched by bh_regexp_matches; hence
# alternation (`a|b`) expresses multiple patterns at once, and a regexp of
# several lines, such as the one bh_filter_add builds, requires every line.
bh_filter_candidate() {
    BH_CANDIDATE=skip
    BH_CANDIDATE_STATIC=0
    if [ -n "${BH_FILTER:-}" ] && ! bh_regexp_matches "$1" "$BH_FILTER"; then
        return 0
    fi
    BH_CANDIDATE=build
    if [ -n "${BH_REBUILD:-}" ] && bh_regexp_matches "$1" "$BH_REBUILD"; then
        BH_CANDIDATE=rebuild
    fi
    if [ -n "${BH_STATIC:-}" ] && bh_regexp_matches "$1" "$BH_STATIC"; then
        BH_CANDIDATE_STATIC=1
    fi
    return 0
}

# bh_regexp_matches <text> <regexp>
#
# Tells whether the extended regexp (`grep -E`) <regexp> finds a match in
# <text>. A <regexp> of several lines matches only when every line does, each
# line being a regexp of its own; that is how bh_filter_add joins two filters.
bh_regexp_matches() {
    _brm_nl='
'
    _brm_rest=$2
    while :; do
        _brm_line=${_brm_rest%%"$_brm_nl"*}
        printf '%s\n' "$1" | grep -Eq -- "$_brm_line" || return 1
        [ "$_brm_line" != "$_brm_rest" ] || return 0
        _brm_rest=${_brm_rest#*"$_brm_nl"}
    done
}

# bh_regexp_text <regexp>
#
# Prints <regexp> quoted for a message: `'<regexp>'`, or, for a regexp of
# several lines (see bh_regexp_matches), each line quoted, joined by `and`.
bh_regexp_text() {
    printf '%s\n' "$1" | awk '
        NR > 1 { printf " and " }
        { printf "\047%s\047", $0 }
    '
}

# bh_filter_add <regexp>
#
# Narrows --filter to the projects that <regexp> accepts too: sets BH_FILTER
# to <regexp> when no --filter was given, else adds <regexp> as one more line,
# which bh_regexp_matches requires as well. --filter's own value is parsed by
# bh_parse_args first; hence a build.sh calls this from an `args-parsed` hook
# (see bh_hook).
bh_filter_add() {
    [ -n "$1" ] || return 0
    BH_FILTER="${BH_FILTER:+$BH_FILTER
}$1"
}

# bh_build_walk <prepare|build>
#
# Walks the order lines of the `.qmake.save_space` cache at the build root, one
# build candidate at a time and in the cache's own order, asking
# bh_filter_candidate about each; nothing is collected first. A project with
# one order line per variant is one candidate, as those lines follow each
# other. `prepare` only counts the candidates the filter keeps into _bw_total,
# and empties the time of every finished line of each `rebuild` candidate, all
# before any is built, so a rebuild cut short leaves every match not yet
# rebuilt unfinished, and the next run, rebuild or not, builds them. `build`
# hands each kept candidate to bh_build_candidate, tallying _bw_made and
# _bw_skipped. Both modes share this one walk, so the count always matches the
# projects the build visits.
bh_build_walk() {
    _bw_mode=$1
    _bw_tab=$(printf '\t')
    _bw_last=
    _bw_total=0 _bw_made=0 _bw_skipped=0
    # Reads the cache on fd 3, so the build keeps stdin free and the loop runs
    # in THIS shell (no pipe subshell) -- so a bh_error propagates and the
    # counters survive. A cache rewritten meanwhile is a new file, which leaves
    # the one being read as it was.
    while IFS= read -r _bw_line <&3; do
        # An order line has exactly three columns.
        _bw_pro=${_bw_line%%"$_bw_tab"*}
        _bw_rest=${_bw_line#*"$_bw_tab"}
        [ "$_bw_rest" != "$_bw_line" ] || continue
        _bw_target=${_bw_rest#*"$_bw_tab"}
        [ "$_bw_target" != "$_bw_rest" ] || continue
        case "$_bw_target" in
            *"$_bw_tab"*) continue ;;
        esac
        if [ -z "$_bw_pro" ] || [ "$_bw_pro" = "$_bw_last" ]; then
            continue
        fi
        _bw_last=$_bw_pro
        # A line may outlive its project.
        [ -f "$_bw_pro" ] || continue
        _bw_rel=${_bw_pro#"$BH_ROOT"/}
        bh_filter_candidate "$_bw_rel"
        [ "$BH_CANDIDATE" != skip ] || continue
        _bw_total=$((_bw_total + 1))
        if [ "$_bw_mode" = build ]; then
            bh_build_candidate "$_bw_pro" "$_bw_rel"
        elif [ "$BH_CANDIDATE" = rebuild ]; then
            bh_save_space_unfinish "$_bw_pro"
        fi
    done 3< "$BUILD_DIR/.qmake.save_space"
}

# bh_build_candidate <pro> <path>
#
# Builds the one candidate bh_build_walk kept, <path> being <pro> relative to
# BH_ROOT: makes it in its shadow build dir, from the Makefile the recorded
# qmake run wrote there, or builds it alone when that Makefile is missing.
# Under --save-space a finished project is skipped.
bh_build_candidate() {
    _bc_pro=$1
    _bc_rel=$2
    if [ "${BH_SAVE_SPACE:-0}" -eq 1 ] && bh_save_space_is_finished "$_bc_pro"; then
        _bw_skipped=$((_bw_skipped + 1))
        return 0
    fi
    _bw_made=$((_bw_made + 1))
    _bc_bd=$BUILD_DIR/$(dirname "$_bc_rel")
    _bc_makefile=$(bh_makefile_of "$_bc_pro")
    # bh_build_alone is where qmake runs, hence --qmake (and/or --refresh)
    # takes this way too, else a Makefile that exists is never regenerated.
    if [ ! -f "$_bc_bd/$_bc_makefile" ] \
            || bh_makefile_mode_differs "$_bc_bd/$_bc_makefile" \
            || bh_makefile_build_differs "$_bc_bd/$_bc_makefile" \
            || bh_static_list_newer "$_bc_bd/$_bc_makefile" \
            || bh_makefile_stale "$_bc_bd/$_bc_makefile" \
            || [ "${BH_FORCE_QMAKE:-0}" -gt 0 ]; then
        bh_build_alone "$_bc_pro" "$_bc_rel" "[$_bw_total/$_bw_count]"
        return 0
    fi
    if [ "${BH_SAVE_SPACE:-0}" -ne 1 ]; then
        if [ "$BH_CANDIDATE" = rebuild ]; then
            bh_build_alone "$_bc_pro" "$_bc_rel" "[$_bw_total/$_bw_count]"
            return 0
        fi
        echo
        echo "$BH_BUILD_LABEL [$_bw_total/$_bw_count]: $_bc_rel"
        ( cd "$_bc_bd" && BH_MAKEFILE=$_bc_makefile bh_make "$JOBS" ) \
            || bh_error "--filter: build failed for $_bc_rel"
        return 0
    fi
    if [ "$BH_CANDIDATE" = rebuild ]; then
        echo "--rebuild: $_bc_rel matches; building it again."
    else
        echo "--save-space: $_bc_rel has no finished build on record; building it again."
    fi
    # Empties the time of this project's finished lines first, so a build that
    # dies before post-link leaves the project unfinished and the next run
    # builds it again.
    bh_save_space_unfinish "$_bc_pro"
    echo "--save-space build [$_bw_total/$_bw_count]: $_bc_rel"
    ( cd "$_bc_bd" && BH_MAKEFILE=$_bc_makefile bh_make "$JOBS" ) \
        || bh_error "--save-space: build failed for $_bc_rel"
    bh_save_space_mark_finished "$_bc_pro" "$_bc_bd/$_bc_makefile"
}

# bh_pro_is_subdirs <pro>
#
# Tells whether <pro> sets `TEMPLATE = subdirs`, so it has no target of its
# own, only subprojects.
bh_pro_is_subdirs() {
    grep -Eq '^[[:space:]]*TEMPLATE[[:space:]]*=[[:space:]]*subdirs[[:space:]]*$' "$1" 2>/dev/null
}

# bh_pro_is_aux <pro>
#
# Tells whether <pro> sets `TEMPLATE = aux`: its qmake run does its work
# (such as writing a descriptor), and it links nothing to mark finished.
bh_pro_is_aux() {
    grep -Eq '^[[:space:]]*TEMPLATE[[:space:]]*=[[:space:]]*aux[[:space:]]*$' "$1" 2>/dev/null
}

# bh_build_alone <pro> <path> [<counter>]
#
# Builds one project alone, <path> being <pro> relative to BH_ROOT: makes it
# in its own out-of-source dir (mirroring the source layout under BUILD_DIR),
# from the Makefile `bh_makefile_of` names for it, after a qmake of just that
# project when none ran for it yet, when its Makefile was made for the other
# --save-space mode, or when --qmake/--refresh asks. A `rebuild` candidate is
# cleaned in its qmake step, unless --save-space (which keeps no objects)
# already asked for it. Under --save-space it then makes sure the project is
# marked finished. Cross-project deps are not pulled in automatically.
# <counter>, such as `[3/9]`, goes into the line that names the project.
bh_build_alone() {
    _ba_bd=$BUILD_DIR/$(dirname "$2")
    _ba_mf=$(bh_makefile_of "$1")
    _ba_force=${BH_FORCE_QMAKE:-0}
    if [ -f "$_ba_bd/$_ba_mf" ] && bh_makefile_mode_differs "$_ba_bd/$_ba_mf"; then
        _ba_force=1
    fi
    _ba_clean=0
    echo
    if [ "$BH_CANDIDATE" = rebuild ] && [ "${BH_SAVE_SPACE:-0}" -ne 1 ]; then
        echo "--rebuild: $2 matches; building it again."
        _ba_clean=1
    fi
    echo "$BH_BUILD_LABEL${3:+ $3}: $2"
    _ba_order=
    if [ "${BH_SAVE_SPACE:-0}" -ne 1 ] \
            && ! BU_PRO=$1 bh_save_space_has_line 'NF == 3 && $1 == ENVIRON["BU_PRO"]'; then
        _ba_order=CONFIG+=save_order
    fi
    # shellcheck disable=SC2086  # intentional word-split of forwarded args.
    ( mkdir -p "$_ba_bd" && cd "$_ba_bd" \
        && BH_MAKEFILE=$_ba_mf BH_FORCE_QMAKE=$_ba_force BH_QMAKE_CLEAN=$_ba_clean \
            bh_run_qmake qmake "$1" $_ba_order $BH_REMAINING_ARGS \
        && BH_MAKEFILE=$_ba_mf bh_make "$JOBS" ) \
        || bh_error "--filter: build failed for $2"
    if [ "${BH_SAVE_SPACE:-0}" -eq 1 ]; then
        bh_save_space_mark_finished "$1" "$_ba_bd/$_ba_mf"
    fi
}

# bh_build_unrecorded [mode-tag]
#
# Goes on, after bh_build_walk, with the projects the `.qmake.save_space`
# cache at the build root does not list: every `.pro` under BH_ROOT, one at a
# time as `find | sort` hands them over, that bh_filter_candidate keeps, that
# has no order line, and that bh_order_reach finds `listed` or `alone` is
# built alone (see bh_build_alone); one that it finds `left-out` is reported
# and skipped. Nothing is collected first. Without --filter only the `rebuild` candidates count, as
# the root project already covers a plain build (and a bare --rebuild, whose
# regexp is `^.*$`, does not come here at all). Under --save-space such a project gets its finished line
# too, so it is skipped from then on, unless --rebuild names it; a subdirs
# project is passed over there, as it has no target to finish and its leaves
# are the projects that build (recorded ones through bh_build_walk, the rest
# here on their own). The loop runs
# at the end of a pipe, in its own shell, so it answers through its exit
# status: 0 when a project was left for it, 10 when none was, and the failed
# step's own status else.
bh_build_unrecorded() {
    _bu_mode=${1:-build}
    # fd 4 keeps the caller's stdin for the build steps, while the loop reads
    # the pipe on fd 3. `|| true`: a `find` that could not read some dir still
    # listed the rest.
    {
        bh_pro_files | bh_filter_paths --relax \
        | {
            _bu_total=0
            while IFS= read -r _bu_pro <&3; do
                _bu_rel=${_bu_pro#"$BH_ROOT"/}
                bh_filter_candidate "$_bu_rel"
                [ "$BH_CANDIDATE" != skip ] || continue
                if [ -z "${BH_FILTER:-}" ] && [ "$BH_CANDIDATE" != rebuild ]; then
                    continue
                fi
                if BU_PRO=$_bu_pro bh_save_space_has_line \
                        'NF == 3 && $1 == ENVIRON["BU_PRO"]'; then
                    continue
                fi
                # Builds only what a subdirs project lists, or what has none
                # above it; bh_build_order_extend already qmaked the rest.
                bh_order_reach "$_bu_pro"
                case "$BH_REACH" in
                    left-out)
                        if [ "$_bu_mode" = build ]; then
                            echo "--filter: $_bu_rel is not listed by ${BH_REACH_PRO#"$BH_ROOT"/}; leaving it out."
                        fi
                        continue
                        ;;
                    qmake) continue ;;
                esac
                _bu_total=$((_bu_total + 1))
                [ "$_bu_mode" = build ] || continue
                if [ "${BH_SAVE_SPACE:-0}" -eq 1 ] && bh_pro_is_subdirs "$_bu_pro"; then
                    echo "--save-space: $_bu_rel is a subdirs project; its subprojects build on their own."
                    continue
                fi
                if [ "${BH_SAVE_SPACE:-0}" -eq 1 ]; then
                    if [ "$BH_CANDIDATE" != rebuild ] \
                            && bh_save_space_is_finished "$_bu_pro"; then
                        echo "--save-space: $_bu_rel finished once; skipping it."
                        continue
                    fi
                    bh_save_space_unfinish "$_bu_pro"
                fi
                bh_build_alone "$_bu_pro" "$_bu_rel" \
                    "[$((_bw_total + _bu_total))/$_bw_count]"
            done
            if [ "$_bu_mode" = count ]; then
                echo "$_bu_total"
                exit 0
            fi
            [ "$_bu_total" -gt 0 ] || exit 10
        } 3<&0 0<&4
    } 4<&0
}

# bh_order_lists <pro>
#
# Tells whether a subdirs project on record in the `.qmake.save_space` cache
# lists <pro> as one of its subprojects (see saveOrderSubdirs() in
# save_order.prf).
bh_order_lists() {
    BU_PRO=$1 bh_save_space_has_line 'NF == 2 && $2 == ENVIRON["BU_PRO"]'
}

# bh_order_qmaked <pro>
#
# Tells whether the subdirs project <pro> has its subprojects on record, from
# a qmake run of it that is newer than <pro>; an older one may list what the
# project no longer does, hence it counts as none.
bh_order_qmaked() {
    BU_PRO=$1 bh_save_space_has_line \
        'NF == 2 && $1 == ENVIRON["BU_PRO"] && $2 == ""' || return 1
    _boq_makefile=$(bh_order_makefile "$1")
    [ -f "$_boq_makefile" ] && [ ! "$1" -nt "$_boq_makefile" ]
}

# bh_order_makefile <pro>
#
# Prints the path of the Makefile that a build's qmake writes for <pro> in its
# shadow build dir: `$BUILD_DIR/Makefile` for a .pro in BH_ROOT itself, which
# the root's qmake writes whatever the .pro's name, else the one
# bh_makefile_of names.
bh_order_makefile() {
    _bom_rel=${1#"$BH_ROOT"/}
    if [ "$(dirname "$_bom_rel")" = . ]; then
        printf '%s\n' "$BUILD_DIR/Makefile"
    else
        printf '%s\n' "$BUILD_DIR/$(dirname "$_bom_rel")/$(bh_makefile_of "$1")"
    fi
}

# bh_pro_parent <pro>
#
# Prints the subdirs project nearest above <pro>: a `TEMPLATE = subdirs` .pro
# in the folder of <pro> (but <pro> itself), else in the nearest folder above
# it, up to BH_ROOT; prints nothing when there is none. Of several in one
# folder, the one named after the folder wins, and in BH_ROOT the build's root
# project (BH_ORDER_ROOT, see bh_build_in_order), since the others there are
# roots of other builds.
bh_pro_parent() {
    _bpp_dir=$(dirname "$1")
    while :; do
        if [ "$_bpp_dir" = "$BH_ROOT" ] && [ -n "${BH_ORDER_ROOT:-}" ]; then
            [ "$BH_ORDER_ROOT" = "$1" ] || printf '%s\n' "$BH_ORDER_ROOT"
            return 0
        fi
        for _bpp_pro in "$_bpp_dir/$(basename "$_bpp_dir").pro" "$_bpp_dir"/*.pro; do
            [ "$_bpp_pro" != "$1" ] && [ -f "$_bpp_pro" ] || continue
            if bh_pro_is_subdirs "$_bpp_pro"; then
                printf '%s\n' "$_bpp_pro"
                return 0
            fi
        done
        [ "$_bpp_dir" != "$BH_ROOT" ] && [ "$_bpp_dir" != / ] || return 0
        _bpp_dir=$(dirname "$_bpp_dir")
    done
}

# bh_order_reach <pro>
#
# Tells, through BH_REACH, whether the build reaches <pro>, a project of the
# source tree that has no order line, by walking up its chain of subdirs
# projects (see bh_pro_parent) and asking the `.qmake.save_space` cache:
#
#   listed     a subdirs project on record lists <pro> (such as a
#              `TEMPLATE = aux` project, which links nothing);
#   left-out   the nearest subdirs project on record above it lists neither
#              <pro> nor the chain down to it, as on another platform;
#   qmake      no qmake has listed that chain yet, hence BH_REACH_PRO, the
#              subdirs project highest up it that is still unknown, has to be
#              qmaked first;
#   alone      no subdirs project lies above <pro> at all.
#
# The build's root project (BH_ORDER_ROOT) lists only what a plain build makes
# (XD's leaves out its tests, say); hence a chain it does not list is no
# left-out one, but a part of the tree of its own, whose top subdirs project
# is qmaked (or <pro> built `alone`, when that is <pro> itself).
#
# Next to `left-out`, BH_REACH_PRO names the project that leaves it out.
bh_order_reach() {
    _bre_cur=$1
    BH_REACH_PRO=
    while :; do
        if bh_order_lists "$_bre_cur"; then
            if [ "$_bre_cur" = "$1" ]; then
                BH_REACH=listed
            else
                BH_REACH=qmake
                BH_REACH_PRO=$_bre_cur
            fi
            return 0
        fi
        _bre_parent=$(bh_pro_parent "$_bre_cur")
        if [ -z "$_bre_parent" ]; then
            if [ "$_bre_cur" = "$1" ]; then
                BH_REACH=alone
            else
                BH_REACH=qmake
                BH_REACH_PRO=$_bre_cur
            fi
            return 0
        fi
        if bh_order_qmaked "$_bre_parent"; then
            if [ "$_bre_parent" != "${BH_ORDER_ROOT:-}" ]; then
                BH_REACH=left-out
                BH_REACH_PRO=$_bre_parent
            elif [ "$_bre_cur" = "$1" ]; then
                BH_REACH=alone
            else
                BH_REACH=qmake
                BH_REACH_PRO=$_bre_cur
            fi
            return 0
        fi
        _bre_cur=$_bre_parent
    done
}

# bh_build_order_extend
#
# Records, before bh_build_walk, the build order of each part of the source
# tree that --filter (or --rebuild) names but no qmake listed yet: for every
# match without an order line whose chain is unknown (see bh_order_reach), it
# qmakes that chain's subdirs project in its shadow build dir the way
# bh_build_order_record qmakes the root, which writes the order lines of its
# whole subtree and the list of each subdirs project in it. Hence bh_build_walk
# then builds those matches in their build order too, and a match that no
# subdirs project lists on this platform is left out. One `.pro` at a time, as
# `find | sort` hands them over; a qmake that fails only warns.
bh_build_order_extend() {
    # fd 4 keeps the caller's stdin for the qmake steps, while the loop reads
    # the pipe on fd 3, as in bh_build_unrecorded.
    {
        bh_pro_files | bh_filter_paths --relax \
        | while IFS= read -r _boe_pro <&3; do
            _boe_rel=${_boe_pro#"$BH_ROOT"/}
            bh_filter_candidate "$_boe_rel"
            [ "$BH_CANDIDATE" != skip ] || continue
            if [ -z "${BH_FILTER:-}" ] && [ "$BH_CANDIDATE" != rebuild ]; then
                continue
            fi
            if BU_PRO=$_boe_pro bh_save_space_has_line \
                    'NF == 3 && $1 == ENVIRON["BU_PRO"]'; then
                continue
            fi
            bh_order_reach "$_boe_pro"
            [ "$BH_REACH" = qmake ] || continue
            _boe_parent_rel=${BH_REACH_PRO#"$BH_ROOT"/}
            _boe_makefile=$(bh_order_makefile "$BH_REACH_PRO")
            # In a subshell, since a failed step exits its shell.
            ( mkdir -p "$(dirname "$_boe_makefile")" \
                && cd "$(dirname "$_boe_makefile")" \
                && BH_MAKEFILE=$(basename "$_boe_makefile") \
                    bh_qmake_in_build_order "$BH_REACH_PRO" CONFIG+=save_order ) \
                || bh_warning "Recording the build order of $_boe_parent_rel failed." \
                    "Its projects that the filters name are left out."
        done 3<&0 0<&4
    } 4<&0
}

bh_filter_has_projects() {
    bh_pro_files | bh_filter_paths --relax | awk 'NR { found = 1 } END { exit !found }'
}

# bh_save_space_has_line <awk-test>
#
# Tells whether the `.qmake.save_space` cache at the build root holds a line
# that <awk-test> accepts, an awk condition over the tab-separated columns.
bh_save_space_has_line() {
    [ -f "$BUILD_DIR/.qmake.save_space" ] || return 1
    awk -F'\t' "$1"' { found = 1 } END { exit !found }' \
        "$BUILD_DIR/.qmake.save_space"
}

# bh_save_space_forget
#
# Drops, from the `.qmake.save_space` cache at the build root, the finished
# lines of every project that bh_filter_candidate keeps: the ones --filter
# accepts, narrowed to the ones --rebuild accepts too when it was given. The
# order lines stay, each in its place, so a forgotten project is built again
# where it was.
bh_save_space_forget() {
    _ssg_cache=$BUILD_DIR/.qmake.save_space
    [ -f "$_ssg_cache" ] || return 0
    _ssg_tab=$(printf '\t')
    _ssg_last=
    _ssg_drop=0
    while IFS= read -r _ssg_line <&3; do
        _ssg_pro=${_ssg_line%%"$_ssg_tab"*}
        # Lines of one project mostly follow each other; ask once per run of
        # them.
        if [ "$_ssg_pro" != "$_ssg_last" ]; then
            _ssg_last=$_ssg_pro
            bh_filter_candidate "${_ssg_pro#"$BH_ROOT"/}"
            _ssg_drop=0
            if [ "$BH_CANDIDATE" = rebuild ] \
                    || { [ "$BH_CANDIDATE" = build ] && [ -z "${BH_REBUILD:-}" ]; }; then
                _ssg_drop=1
            fi
        fi
        # A finished line has a fourth column.
        case "$_ssg_line" in
            *"$_ssg_tab"*"$_ssg_tab"*"$_ssg_tab"*)
                [ "$_ssg_drop" -eq 0 ] || continue
                ;;
        esac
        printf '%s\n' "$_ssg_line"
    done 3< "$_ssg_cache" > "$_ssg_cache.tmp"
    mv -f "$_ssg_cache.tmp" "$_ssg_cache"
}

# bh_save_space_unfinish <pro>
#
# Empties, in place, the time of every finished line of <pro> in the
# `.qmake.save_space` cache at the build root. No line moves; the project then
# counts as not finished.
bh_save_space_unfinish() {
    _ssu_cache=$BUILD_DIR/.qmake.save_space
    awk -F'\t' -v OFS='\t' -v pro="$1" '
        NF >= 4 && $1 == pro { $4 = "" }
        { print }
    ' "$_ssu_cache" > "$_ssu_cache.tmp" && mv -f "$_ssu_cache.tmp" "$_ssu_cache"
}

# bh_save_space_mark_finished <pro> <makefile>
#
# Makes sure <pro>, just built from <makefile>, counts as finished (an aux
# project excepted, which never does): when its
# post-link wrote no finished line (the build linked nothing, its target being
# up to date already), it asks <makefile> for them through the
# `save_space_finished` target of save_space.prf. Then it drops the emptied
# finished lines of <pro> (see bh_save_space_drop_unfinished). Fails when a
# target recorded for <pro> is missing, as the build then failed.
bh_save_space_mark_finished() {
    # Leaves an aux project unmarked: it has no target, and each build runs its
    # qmake again, which redoes its work.
    if bh_pro_is_aux "$1"; then
        return 0
    fi

    # A make that failed without saying so leaves a target missing; that is a
    # failed build, never a finished one.
    _smf_missing=$(bh_save_space_order_targets "$1" | cut -f2 \
        | while IFS= read -r _smf_target; do
            [ -z "$_smf_target" ] || [ -f "$_smf_target" ] || printf '%s\n' "$_smf_target"
        done)
    if [ -n "$_smf_missing" ]; then
        bh_error "--save-space: ${1#"$BH_ROOT"/} built no $(printf '%s' "$_smf_missing" | head -1)"
    fi
    if ! bh_save_space_is_finished "$1"; then
        ( cd "$(dirname "$2")" \
            && bh_run --whole save-space-finished bh_make_run -f "$(basename "$2")" save_space_finished ) \
            || bh_error "--save-space: cannot mark ${1#"$BH_ROOT"/} finished"
    fi
    bh_save_space_drop_unfinished "$1"
}

# bh_save_space_drop_unfinished <pro>
#
# Drops, from the `.qmake.save_space` cache at the build root, the finished
# lines of <pro> whose time was emptied, once <pro> built: post-link has
# appended a fresh finished line for each variant by then, so the emptied ones
# would only repeat it.
bh_save_space_drop_unfinished() {
    _ssn_cache=$BUILD_DIR/.qmake.save_space
    awk -F'\t' -v pro="$1" '!(NF >= 4 && $1 == pro && $4 == "")' \
        "$_ssn_cache" > "$_ssn_cache.tmp" && mv -f "$_ssn_cache.tmp" "$_ssn_cache"
}

# bh_save_space_order_targets <pro>
#
# Prints "<variant>\t<target>" for each variant the order lines of <pro> in
# the `.qmake.save_space` cache at the build root name, in their order. Only
# the build mode of <pro>'s latest order line counts: a qmake run in another
# mode records the project again, a debug_and_release one with
# `debug_and_release:<variant>` tags and a single-config one with a bare
# `<variant>`, so the lines of the other mode are left over. A variant
# recorded more than once counts once, with its latest target.
bh_save_space_order_targets() {
    awk -F'\t' -v pro="$1" '
        NF == 3 && $1 == pro {
            if (!($2 in slot)) {
                slot[$2] = ++n
                tag[n] = $2
            }
            path[slot[$2]] = $3
            dual = ($2 ~ /:/)
        }
        END {
            for (i = 1; i <= n; i++) {
                if ((tag[i] ~ /:/) == dual) {
                    printf "%s\t%s\n", tag[i], path[i]
                }
            }
        }
    ' "$BUILD_DIR/.qmake.save_space"
}

# bh_save_space_is_finished <pro>
#
# Tells whether <pro> finished once: the `.qmake.save_space` cache at the build
# root has a finished line, with a time, for every variant its order lines
# name (see bh_save_space_order_targets; or for any variant, when it has no
# order line), and each recorded target is still there.
bh_save_space_is_finished() {
    _ssf_targets=$({
        bh_save_space_order_targets "$1" | awk -F'\t' -v OFS='\t' '{ print "O", $0 }'
        awk -F'\t' -v OFS='\t' -v pro="$1" \
            'NF >= 4 && $1 == pro && $4 != "" { print "F", $2, $3 }' \
            "$BUILD_DIR/.qmake.save_space"
    } | awk -F'\t' '
        $1 == "O" { variant[++n] = $2; target[n] = $3; next }
        $1 == "F" { finished[$2] = $3 }
        END {
            if (n == 0) {
                for (each in finished) {
                    variant[++n] = each
                    target[n] = finished[each]
                }
            }
            if (n == 0) {
                exit 1
            }
            for (i = 1; i <= n; i++) {
                if (target[i] == "" || !(variant[i] in finished)) {
                    exit 1
                }
            }
            for (i = 1; i <= n; i++) {
                print target[i]
            }
        }
    ') || return 1
    while IFS= read -r _ssf_target; do
        [ -f "$_ssf_target" ] || return 1
    done <<EOF
$_ssf_targets
EOF
    return 0
}

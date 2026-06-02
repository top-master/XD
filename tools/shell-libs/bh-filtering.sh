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

# bh_pro_is_subdirs <pro>
#
# Tells whether <pro> sets `TEMPLATE = subdirs`, so it has no target of its
# own, only subprojects.
bh_pro_is_subdirs() {
    grep -Eq '^[[:space:]]*TEMPLATE[[:space:]]*=[[:space:]]*subdirs[[:space:]]*$' "$1" 2>/dev/null
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

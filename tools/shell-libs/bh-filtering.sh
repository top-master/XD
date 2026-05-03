# License: Apache 2.0 without attribution need.
#
# Build order and filters of `build-handler.sh`, which sources this file after
# its own helpers: the `.qmake.save_space` cache that save_order.prf and
# save_space.prf fill, the walk of its order lines, and --filter, --rebuild,
# and --save-space on top of that order. build-handler.md describes them.


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


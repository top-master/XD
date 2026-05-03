# License: Apache 2.0 without attribution need.
#
# --very-verbose helpers of `build-handler.sh`, which sources this file after
# its own helpers: the trace of what the qmake skip check of `bh_run_qmake`
# compared, with the files' modified-times.


# bh_file_time <file>
#
# Prints the modified-time of <file> as `stat` tells it, to the nanosecond
# where the file system keeps it, or `missing` when there is no such file.
bh_file_time() {
    if [ ! -e "$1" ]; then
        echo missing
        return 0
    fi
    stat -c '%y' "$1" 2>/dev/null \
        || stat -f '%Sm' -t '%Y-%m-%d %H:%M:%S %z' "$1"
}


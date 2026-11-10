#!/bin/sh
# License: Apache 2.0 without attribution need.
#
# test-result.sh <result-file> <test-binary> <command>
#
# Runs one test's `make check` command (see mkspecs/xd/test_result.prf) and
# writes `passed` or `failed` into <result-file>, then exits with the test's
# status. With XD_TEST_FAILED_ONLY=1 in the environment (build.sh --failed),
# it skips a test whose last run passed and whose binary was not built again
# since, hence a rerun only runs what failed, or what changed.

result=$1
binary=$2
command=$3

if [ "${XD_TEST_FAILED_ONLY:-0}" = 1 ] && [ -f "$result" ] \
        && [ "$(cat "$result")" = passed ] && [ ! "$binary" -nt "$result" ]; then
    echo "Skipping $binary: it passed, and was not built again since."
    exit 0
fi

eval "$command"
status=$?
if [ "$status" -eq 0 ]; then
    echo passed > "$result"
else
    echo failed > "$result"
fi
exit "$status"

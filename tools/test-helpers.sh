#!/bin/sh

# License: Apache 2.0 without attribution need.

# ---- test-helpers.sh ---------------------------------------------------
#
# QtTestLib-styled assertion helpers for shell self-tests. Drop this
# beside a `*.spec.sh` file (or any sh-based test runner), source it,
# and the test report comes out in the same `PASS   : ...` /
# `Totals: ...` / `********* Finished testing of ... *********` shape
# that QtTest's plain-text logger emits -- so build-handler.spec.sh,
# QtTest binaries, and future shell specs all stream into the same
# eyeball pattern.
#
# Usage:
#
#     . "$_dir/test-helpers.sh"
#     test_begin 'my-suite'                  # banner + start clock
#
#     test_section 'feature A -- defaults'   # group separator
#     expect 'desc' "$expected" "$actual"    # string equality
#     expect_int_gt 'desc' "$actual" "$min"  # actual > floor
#     expect_returns 'desc' 0 my_func arg    # exit status of <cmd>
#
#     test_end                               # totals + duration + exit
#
# Section separators (`== ... ==`) are a non-QtTest scannability
# layer kept on top -- QtTest groups silently by test-class, while
# shell specs lump many assertions per file and benefit from headings.
#
# Exit status: number of failed assertions (0 = all pass). CI wires
# straight onto that.

_th_total=0
_th_failed=0
_th_name=
_th_start_ms=0

# Best-effort high-res clock. Perl's Time::HiRes ships with macOS /
# stock Linux perl; if it's missing we fall back to whole-second
# `date +%s` and the Duration line just shows `.000` for ms.
_th_now_ms() {
    perl -MTime::HiRes=time -e 'printf "%d\n", time * 1000' 2>/dev/null \
        || echo "$(($(date +%s) * 1000))"
}

# Internal: PASS/FAIL row primitives. QtTest pads its keyword field
# to 7 chars + `: ` so colons align; we match that. The diff lines
# under FAIL!  use 9-space indent so the keyword column lines up
# under PASS's text.
_th_pass() { printf 'PASS   : %s\n' "$1"; }
_th_fail_kv() {
    printf 'FAIL!  : %s\n         %s: %s\n         %s: %s\n' \
        "$1" "$2" "$3" "$4" "$5"
}

# test_begin <suite-name>
# Print the QtTest-style "Start testing of ..." banner and arm the
# duration clock. Call once per file, before any assertions.
test_begin() {
    _th_name=$1
    _th_start_ms=$(_th_now_ms)
    printf '********* Start testing of %s *********\n' "$_th_name"
    printf 'Config: %s self-test on %s\n' "$_th_name" "$(uname -srm)"
}

# test_section <name>
# Print a `== name ==` separator. Optional but recommended; QtTest
# itself has no equivalent, but shell specs cram many assertions per
# file and headings keep the report scannable.
test_section() {
    printf '\n== %s ==\n' "$1"
}

# expect <description> <expected> <actual>
# String equality assertion.
expect() {
    _th_total=$((_th_total + 1))
    if [ "$2" = "$3" ]; then
        _th_pass "$1"
    else
        _th_failed=$((_th_failed + 1))
        _th_fail_kv "$1" 'expected' "[$2]" 'actual  ' "[$3]"
    fi
}

# expect_int_gt <description> <actual> <floor>
# Numeric assertion: actual must be a positive integer greater than
# <floor>. Echoes the value on success so the report doubles as a
# crude readout of host-specific numbers (CPU count, etc.).
expect_int_gt() {
    _th_total=$((_th_total + 1))
    if [ "$2" -gt "$3" ] 2>/dev/null; then
        printf 'PASS   : %s (= %s)\n' "$1" "$2"
    else
        _th_failed=$((_th_failed + 1))
        _th_fail_kv "$1" 'expected' "> $3" 'actual  ' "[$2]"
    fi
}

# expect_returns <description> <expected-status> <cmd...>
# Runs <cmd> in a subshell with stdout/stderr silenced; asserts on
# the exit code. The subshell isolation means a failing helper can't
# pollute the spec's running totals or leak partial state.
expect_returns() {
    _th_desc=$1
    _th_expected=$2
    shift 2
    _th_total=$((_th_total + 1))
    ( "$@" ) >/dev/null 2>&1
    _th_actual=$?
    if [ "$_th_actual" -eq "$_th_expected" ]; then
        _th_pass "$_th_desc"
    else
        _th_failed=$((_th_failed + 1))
        _th_fail_kv "$_th_desc" \
            'expected exit' "$_th_expected" \
            'actual exit  ' "$_th_actual"
    fi
}

# test_end
# Emits Totals / Duration / "Finished testing of ..." and exits with
# the failure count. Call once at the end of the spec file.
test_end() {
    _th_end_ms=$(_th_now_ms)
    _th_elapsed=$((_th_end_ms - _th_start_ms))
    _th_mm=$((_th_elapsed / 60000))
    _th_ss=$(( (_th_elapsed / 1000) % 60 ))
    _th_msec=$((_th_elapsed % 1000))
    _th_passed=$((_th_total - _th_failed))

    echo
    printf 'Totals: %d passed, %d failed, 0 skipped, 0 blacklisted\n' \
        "$_th_passed" "$_th_failed"
    printf 'Duration: %02d:%02d.%03d second(s)\n' \
        "$_th_mm" "$_th_ss" "$_th_msec"
    printf '********* Finished testing of %s *********\n' "$_th_name"
    exit "$_th_failed"
}

# License: Apache 2.0 without attribution need.
#
# Compile checks of `build-handler.sh`, which sources this file after its own
# helpers: small programs a toolchain must build before a build relies on it,
# as CMake's `check_cxx_source_compiles` does. build-handler.md describes them.


# Args: <compiler>
bh_compile_check_cxx() {
    _bccc_compiler=$1
    _bccc_hash=$(bh_compile_check_key "$_bccc_compiler") || _bccc_hash=
    _bccc_mark=${XDG_CACHE_HOME:-$HOME/.cache}/build-handler/compile-checks/cxx-$_bccc_hash
    if [ -n "$_bccc_hash" ] && [ -f "$_bccc_mark" ]; then
        return 0
    fi
    _bccc_dir=$(mktemp -d) || return 1
    printf '%s\n' '#include <cstddef>' '#include <string>' \
        'int main() { std::string text("x"); return int(text.size()) - 1; }' \
        > "$_bccc_dir/check.cpp"
    "$_bccc_compiler" "$_bccc_dir/check.cpp" -o "$_bccc_dir/check" \
        > "$_bccc_dir/check.log" 2>&1
    _bccc_status=$?
    rm -rf "$_bccc_dir"
    if [ "$_bccc_status" -ne 0 ]; then
        return 1
    fi
    # Only a passed check is kept; a failed one runs again next time.
    if [ -n "$_bccc_hash" ]; then
        mkdir -p "${_bccc_mark%/*}" && : > "$_bccc_mark"
    fi
    return 0
}

# Args: <compiler>
bh_compile_check_key() {
    # The same binary may sit in two toolchains, of which only one has the
    # headers it needs; hence the key names where it sits, not only what it is.
    _bcck_real=$(bh_real_path "$1")
    _bcck_binary=$(bh_sha256_file "$_bcck_real") || return 1
    printf '%s\n%s\n' "$_bcck_real" "$_bcck_binary" | bh_sha256_stdin
}

# Args: <file>
bh_sha256_file() {
    [ -f "$1" ] || return 1
    if command -v sha256sum > /dev/null 2>&1; then
        sha256sum "$1" | cut -d' ' -f1
    elif command -v shasum > /dev/null 2>&1; then
        shasum -a 256 "$1" | cut -d' ' -f1
    else
        openssl dgst -sha256 "$1" | sed 's/.*= *//'
    fi
}

bh_sha256_stdin() {
    if command -v sha256sum > /dev/null 2>&1; then
        sha256sum | cut -d' ' -f1
    elif command -v shasum > /dev/null 2>&1; then
        shasum -a 256 | cut -d' ' -f1
    else
        openssl dgst -sha256 | sed 's/.*= *//'
    fi
}

# Args: <path>
bh_real_path() {
    readlink -f "$1" 2>/dev/null || printf '%s\n' "$1"
}

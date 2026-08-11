# Link metadata and transitive dependencies (`.prl`)

When project A links library B, A must also satisfy everything B depends on. For
a SHARED B (a `.dll` or `.so`) the loader resolves B's own dependencies, so A
links only B. For a STATIC B (a `.a`) there is no such resolution: A's final link
must name every one of B's transitive dependencies explicitly, or it fails with
undefined references. qmake records that per-library list in a `.prl` file.

## The `.prl` file

Every Qt library writes a `<name>.prl` beside it in the install `lib/`. The key
field is `QMAKE_PRL_LIBS`: what a consumer must add when it links this library.
`QMAKE_PRL_CONFIG` records the CONFIG it was built with (see
[load order](./load-order.md) for how that is used to detect foreign-platform
leftovers). Two flags, both set in `features/qt_build_config.prf`, gate the
machinery:

- `create_prl`: this library writes its `.prl`.
- `link_prl`: when linking a dependency, read its `.prl` and pull in its
  `QMAKE_PRL_LIBS`.

## What goes into `QMAKE_PRL_LIBS`: `staticlib` vs `explicitlib`

The writer is `qmake/generators/makefile.cpp` (around line 1065):

    if (isActiveConfig("staticlib") || isActiveConfig("explicitlib")) {
        libs = QMAKE_INTERNAL_PRL_LIBS   // or "QMAKE_LIBS"
        if (isActiveConfig("staticlib"))
            libs += QMAKE_LIBS_PRIVATE   // private deps, STATIC only
        write "QMAKE_PRL_LIBS = " libs
    }

So:

- A STATIC lib ALWAYS lists its transitive deps in the prl, including its
  **private** ones (`QMAKE_LIBS_PRIVATE`). This is how the static
  `Qt5PlatformSupport`'s freetype dependency (`-lqtfreetype`, added by
  `src/3rdparty/freetype_dependency.pri` as `LIBS_PRIVATE`) reaches the platform
  plugins that link it. No `explicitlib` is involved.
- A SHARED lib writes its prl libs only when `explicitlib` is set.

## Why `explicitlib` is unix-only (and why that is correct)

`qt_build_config.prf` sets it with:

    # Under Windows, this is neither necessary (transitive deps are automatically
    # resolved), nor functional (.res files end up in .prl files and break things).
    unix: CONFIG *= explicitlib

On Windows this is deliberate, not a bug:

- A shared Windows DLL is self-contained: its import library pulls the DLL, which
  already carries its own dependencies, so a consumer does not need the DLL's
  transitive list. A shared lib's prl is therefore empty on Windows.
- STATIC libs still propagate their deps regardless, through the `staticlib`
  branch above. So turning `explicitlib` off on Windows does NOT drop static-lib
  transitive deps.

This last point is a trap for the unwary: a missing static transitive dep (for
example a plugin failing with `undefined reference to FT_Load_Glyph`) looks like
`explicitlib` should be enabled for MinGW, but it is not the cause. Enabling it
would only add risk (the `.res`-in-prl breakage the comment warns about) for no
gain, because the `staticlib` branch already emits those libs. The real cause is
almost always a stale prl.

## The regeneration trap (the real cause)

A `.prl` is written when qmake GENERATES the Makefile, not when `make` links the
library. So if `make` relinks a library without re-running qmake (its Makefile
was not invalidated), the `.prl` keeps what it said last time. Because the
install `lib/` is shared and reused across builds and configs, a stale prl can
linger and shadow the correct one: a lib rebuilt with a new dependency gets a
fresh `.a` but an OLD prl that predates that dependency, so consumers never link
it.

The MinGW cross build hit exactly this. `libQt5PlatformSupport.a` was rebuilt
with `qfontengine_ft.o` (the freetype font engine), but `lib/Qt5PlatformSupport.prl`
was a day older and listed only `-lQt5Core`, so the platform plugins failed with
`undefined reference to FT_Load_Glyph`. A clean regeneration (a from-scratch
build, or clearing the stale prl so qmake rewrites it) produced the correct prl:

    QMAKE_PRL_LIBS = ... $$PWD/libQt5Guid.a $$PWD/libQt5DBusd.a \
                     $$PWD/libQt5Cored.a $$PWD/libqtfreetyped.a

Note that the freetype entry is a full path (`$$PWD/libqtfreetyped.a`), not a
`-l` flag, so grepping only for `-lqtfreetype` misses it.

The `.prl` files under `lib/` are **auto-generated** (they are not tracked in
git), so clearing them is safe: qmake rewrites them on the next configure. When
switching platforms in the single-platform `lib/` and `bin/`, clear stale link
metadata so a fresh, consistent set is generated rather than the previous
platform's or a previous config's.

## How `QT += foo-private` finds all this

`QT += platformsupport-private` resolves through the generated module file
`mkspecs/modules/qt_lib_platformsupport_private.pri`, which records the module
NAME (`Qt5PlatformSupport`), its lib DIR, its Qt-module `depends`, and
`module_config = ... staticlib internal_module`. It does NOT list the plain-C
transitive libs; those come from the `.prl` at link time. So the module `.pri`
answers "which Qt lib, and where", and the `.prl` answers "and what else must I
link".

## Related: `save_space` and Windows import libraries

`build.sh --save-space` drops each target's intermediates right after it links
(`mkspecs/xd/save_space.prf`, include()d by `features/default_post.prf`). It must
NOT run the Makefile's `clean` target on Windows: a shared lib links with
`--out-implib lib<name>.a`, and `clean` deletes that import library along with
the objects. A dependent DLL still has to link against it and is not rebuilt, so
`clean` would break the next link. (On unix the single `.so` is both the library
and its link input, so this never surfaced.) `save_space.prf` therefore deletes
the objects and compiler-generated files directly, keeping both the target and
its import library. This is the same "static/import link input must survive"
concern as the `.prl` above, one layer down.

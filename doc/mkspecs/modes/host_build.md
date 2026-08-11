# host_build

`host_build` is a build mode compiled into `qmake` itself (not a plain `.prf`
convention). It marks a project as a *host tool*: something built with the
native toolchain and run on the build machine *during* the build, even while the
rest of the tree cross-compiles for another platform.

The `qmake` bootstrap, `moc`, `rcc`, and `uic` are all host tools. They generate
code, for example `moc header.h` produces `moc_header.cpp`, that the target
compile then consumes; that generated output is platform-neutral, so the tool
itself has to be native (it never needs to be the target binary). This is why a
cross build does not run these under Wine or an emulator, only the final target
artifacts do (see `~/AGENT-tools/docker/wine/wine.md`).

## How `qmake` supports it (built-in)

### 1. Opting in: `option(host_build)`

A tool's `.pro` opts in on its first line:

    option(host_build)

`option()` is handled while `qmake` sets the project up, so the host-build flag
is known before the spec and features load. In XD you see it at the top of
`qmake/qmake.pro`, `src/tools/moc/moc.pro`, `src/tools/rcc/rcc.pro`, and
`src/tools/uic/uic.pro`.

### 2. `host_build` is a virtual scope

`host_build` is never a normal entry in `CONFIG`. `qmake` special-cases the
name: `QMakeEvaluator::isActiveConfig()` returns the internal `m_hostBuild` flag
for it (`qmake/library/qmakeevaluator.cpp`):

    if (config == statics.strhost_build)
        return m_hostBuild;

So `host_build { ... }` and `!host_build { ... }` scopes work by consulting
`m_hostBuild` directly, regardless of what is (or is not) in `CONFIG`.

### 3. Spec selection: `-spec` versus `-xspec`

This is the important part. `qmake` keeps two specs:

- `qmakespec`, the *host* spec, set by `-spec` (alias `-platform`), and
- `xqmakespec`, the *target* spec, set by `-xspec` (alias `-xplatform`).

`QMakeEvaluator::loadSpec()` chooses between them by the host-build flag
(`qmake/library/qmakeevaluator.cpp`):

    QString qmakespec = m_option->expandEnvVars(
                m_hostBuild ? m_option->qmakespec : m_option->xqmakespec);

So a `host_build` project loads the **host** spec, and every other project loads
the **target** spec. The option parsing lives in
`qmake/library/qmakeglobals.cpp`: `-spec` / `-platform` set `qmakespec`, and
`-xspec` / `-xplatform` set `xqmakespec`. If no `-xspec` is given, `xqmakespec`
falls back to `qmakespec` (so a plain `-spec win32-g++` makes host and target the
same spec), and failing that to the `XQMAKESPEC` environment variable.

That fallback is the gotcha. A naive win32 cross build that passes only
`-spec win32-g++` drags the host tools through the target spec too: host and
target both resolve to win32-g++, so `moc` and friends inherit the target's
flags (and, being non-native, fail). The fix is to pass both specs:

    qmake -spec <host-spec> -xspec win32-g++

Now `host_build` tools load `<host-spec>` (native) and only the target loads
win32-g++. XD's `build.sh --cross-compile` (alias `--cross-build`) wires exactly
this: it keeps `-spec` at the native host spec and passes the target as a
separate `-xspec` (default `win32-g++`).

### 4. The compiler: `CROSS_COMPILE`

A cross spec names its tools with a prefix, for example
`QMAKE_CXX = $${CROSS_COMPILE}g++`. `features/device_config.prf` empties that
prefix for host builds:

    host_build {
        CROSS_COMPILE =
    }

so `$${CROSS_COMPILE}g++` collapses to plain `g++` (the native compiler) for a
host tool, while a target build keeps its prefix, for example
`x86_64-w64-mingw32-g++`.

## Consequences worth remembering

- Host tools build with the native compiler and run natively; their output is
  platform-neutral, so where they run has no effect on the target artifact.
- A dependency on the qmake "bootstrap" is a dependency on the **host** qmake:
  `qmake` has to execute on the build machine to generate the Makefiles, read the
  build tree, and drive the cross-compiler. It can never be the target binary.
- Because `host_build` swaps the whole spec, the *target* spec (for example
  win32-g++) is, in a correct setup, only ever loaded for non-host projects. A
  `host_build`-based test written inside the target spec is therefore
  meaningless there; a decision that must apply to both host and target belongs
  in a shared pre-load place instead (see [load order](../load-order.md)).

## Host codegen tools should bootstrap, not link the full host Qt

The host codegen tools (`moc`, `rcc`, `qlalr`, `qdbusxml2cpp`, `qdbuscpp2xml`)
are `host_build`, and they also carry `CONFIG += force_bootstrap` so they link
the small static `Qt5Bootstrap` (and `Qt5BootstrapDBus`) instead of the full
shared host `Qt5Core` / `Qt5DBus`. In XD this is per-tool and explicit: a tool
that omits `force_bootstrap` falls to its `else: QT += <full module>` branch.

That matters in a cross build. A cross build never produces a host Qt, so a
non-bootstrapped host tool links whatever host `Qt5Core.so` happens to be in
`lib/` -- which, in a cross container, was very likely built by a DIFFERENT
compiler than the one in the image. The mismatch shows up as a link error like
`undefined reference to __cxa_call_terminate@CXXABI_1.3.15` (the host lib needs a
newer libstdc++ than the cross image ships). Bootstrapping avoids the host Qt
entirely, so the tool builds against the image's own compiler.

Two related host-tool rules:

- **Gate a host tool on the HOST, not the target.** `idc` (ActiveQt's tool) is
  `host_build` and `#include <windows.h>`, so it can only build when the *host*
  is Windows. Gating it on `win32` (the target) builds it during a Linux-to-win32
  cross and fails to find the header; gate on
  `equals(QMAKE_HOST.os, Windows)` instead.
- If `force_bootstrap` is set on a tool, make sure the matching bootstrap library
  is actually built: XD builds `src_tools_bootstrap_dbus` only under
  `force_bootstrap|private_tests` (see `src/src.pro`).

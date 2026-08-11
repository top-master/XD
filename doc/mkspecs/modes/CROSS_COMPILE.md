# CROSS_COMPILE

`CROSS_COMPILE` is the tool-name prefix a cross spec prepends to its binaries.
`win32-g++/qmake.conf` never hard-codes a compiler; it writes

    QMAKE_CC   = $${CROSS_COMPILE}gcc
    QMAKE_CXX  = $${CROSS_COMPILE}g++
    QMAKE_RC   = $${CROSS_COMPILE}windres

so with `CROSS_COMPILE = x86_64-w64-mingw32-` the toolchain resolves to
`x86_64-w64-mingw32-gcc` and friends, and with an empty prefix it collapses to
the plain native `gcc`, `g++`, `windres`.

Unlike `host_build` (a mode compiled into `qmake`, see [host_build](./host_build.md)),
`CROSS_COMPILE` is not special to `qmake` itself. It is a plain variable that the
mkspec convention reads, in `features/device_config.prf`:

    host_build {
        CROSS_COMPILE =
    } else: isEmpty(CROSS_COMPILE) {
        # this variable can be persisted via qmake -set CROSS_COMPILE /foo
        CROSS_COMPILE = $$[CROSS_COMPILE]
    }

Two things happen here. A `host_build` project empties the prefix, so the host
tools (`moc`, `rcc`, `uic`, the `qmake` bootstrap) build with the native
compiler even while the tree cross-compiles. Every other project, if the
variable is not already set, falls back to the **property** `$$[CROSS_COMPILE]`.

## The three channels, and why only one reaches sub-projects

A recursive build (`qmake -r`) walks the whole `SUBDIRS` tree. How you supply
`CROSS_COMPILE` decides whether the sub-projects ever see it:

| channel | how | reaches sub-projects? |
|---------|-----|-----------------------|
| property | `qmake -set CROSS_COMPILE x86_64-w64-mingw32-` | **yes** |
| command-line variable | `qmake CROSS_COMPILE=x86_64-w64-mingw32-` | no, top project only |
| environment variable | `export CROSS_COMPILE=...` | no, never read |

- The **property** is stored in `qmake`'s persistent settings (`qmake -query`
  shows it), and `device_config.prf` reads it with `$$[CROSS_COMPILE]` while
  evaluating *each* project. So it applies uniformly across `qmake -r`. This is
  the channel to use for a full cross build.
- A **command-line variable** assignment sets the variable for the top-level
  project only. Sub-projects are re-evaluated fresh, where the variable is empty
  again, so line 12 above reads the property instead, which (if it was never
  set) is empty. The sub-makefiles then keep the native `gcc`/`g++`/`windres`
  and the target compile fails (for example `windres: No such file`).
- An **environment variable** is never consulted here: the fallback is the
  property `$$[...]`, not `$$(...)`. Setting the shell variable has no effect.

## Setting it

Persist the property once, then build:

    qmake -set CROSS_COMPILE x86_64-w64-mingw32-
    qmake -r -spec <host-spec> -xspec win32-g++    # then make

`configure`'s `-device-option CROSS_COMPILE=<prefix>` records the same value
through `qdevice.pri` (loaded at the top of `device_config.prf`), which is the
supported path when configuring a device build. Clear a stale value with
`qmake -unset CROSS_COMPILE`.

`deviceSanityCheckCompiler()` (further down `device_config.prf`) is the guard
that turns a missing prefix into a readable error, pointing back at this
variable, rather than a raw "command not found" from `make`.

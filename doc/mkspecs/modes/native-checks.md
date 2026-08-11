# Natively-checked names

Most names in a scope (`win32:`, `opengl:`) or a variable are ordinary: qmake
resolves them by looking in `CONFIG` or in the variable map, both of which the
specs and features fill in. A handful are different: `qmake` recognizes them in
its own C++ before any `.prf` runs, so they behave the same no matter what the
mkspecs do. Those are collected here. [host_build](./host_build.md) and
[CROSS_COMPILE](./CROSS_COMPILE.md) get their own pages; this is the rest.

## Scope names hard-coded in `isActiveConfig()`

`QMakeEvaluator::isActiveConfig()` (`qmake/library/qmakeevaluator.cpp`) is what
decides whether a scope like `foo:` is active. Before it ever consults `CONFIG`
it short-circuits four cases:

    if (config == statics.strtrue)  return true;
    if (config == statics.strfalse) return false;
    if (config == statics.strhost_build) return m_hostBuild;
    // ... then: does `config` match the spec name? (wildcards allowed)
    // ... only then: is `config` in the CONFIG variable?

- `true` and `false` are magic scopes that always take or skip their branch,
  independent of `CONFIG`. Handy for `true: SOURCES += ...` and for temporarily
  wiring a block off with `false:`.
- `host_build` is the only build mode special-cased here; it reads the internal
  `m_hostBuild` flag, never `CONFIG`. See [host_build](./host_build.md).
- The **spec name itself is a scope**. The active spec (for example `win32-g++`,
  `linux-g++`) matches as though it were in `CONFIG`, and the match is a wildcard
  when the scope contains `*` or `?`. So `win32-g++:`, `*-g++:`, and `win32-*:`
  are all real, spec-driven scopes, resolved before the `CONFIG` lookup and
  therefore available as early as any spec-time file (`qconfig.pri` included).
  This is unlike the platform scopes `win32` / `winrt` / `msvc`, which only enter
  `CONFIG` later, at `spec_post.prf` (see [load order](../load-order.md)).

## Variables special-cased on assignment

When a project assigns certain variables, `qmake` reacts immediately rather than
just storing the value (the tail of `QMakeEvaluator::applyVarOperations` /
`writeVar`):

| variable | effect on assignment |
|----------|----------------------|
| `TEMPLATE` | runs `setTemplate()`, normalizing and applying any `-t` override |
| `QMAKE_PLATFORM` | drops the cached feature roots, so `.prf` search is recomputed |
| `QMAKESPEC` | if set to an absolute path, re-points the active spec and drops feature roots |
| `REQUIRES` | runs `checkRequirements()` (full qmake only), which can disable the project |

These fire the moment the line is evaluated, which is why, for example, changing
`QMAKE_PLATFORM` after the spec has loaded does not retroactively rewrite
`CONFIG`: only the feature-root cache is touched, not the platform scopes that
`spec_post.prf` already appended.

## What is *not* native

`CROSS_COMPILE` reads like a built-in but is not: `qmake` has no special handling
for it. It is a plain variable that the mkspec convention
(`features/device_config.prf`) reads and, for host builds, empties. Its behavior
across a recursive build depends entirely on how you pass it, which is why it has
its own page: [CROSS_COMPILE](./CROSS_COMPILE.md).

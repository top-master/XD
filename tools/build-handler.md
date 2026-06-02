# build-handler.sh — the build helper, explained simply

`build-handler.sh` is a big helper script. Think of it as a **robot helper**
that knows how to put the XD app (or a test) together and then run it. You
don't run this file by itself. Instead, a small `build.sh` file "borrows" the
robot's skills and tells it what to do.

So when you type `./build.sh ...`, you are really talking to this robot.

You add little words after `./build.sh` (we call them **flags**, they start
with `--`) to tell the robot what you want. For example `--release` means
"make the fast version", and `--headless` means "don't ask me any questions".

---

## Part 1 — Things you can do (start here)

These are the everyday commands. Run them from the XD project's main folder
(the one that has `build.sh` inside it).

### Build the app

```sh
./build.sh
```

The robot reads the project, makes a plan, and builds it. Simple.

### Build the fast version

```sh
./build.sh --release
```

Same as above, but the "release" (fast) version instead of the "debug"
(easy-to-inspect) version.

### Build and run the tests

```sh
./build.sh --test tests/auto/tools/qmakelib/qmakelib.pro
```

The robot builds things, then builds that **test program** and runs it. A test
program is a little program that checks the real code is behaving. If something
is wrong, it tells you.

### The big example, taken apart

```sh
./build.sh --test tests/auto/tools/qmakelib/qmakelib.pro --headless --verbose --no-build
```

Read left to right, this says:

- `--test tests/auto/tools/qmakelib/qmakelib.pro`
  → "Run **this** test program." (the file path points at one test)
- `--headless`
  → "Don't stop to ask me anything. Just go." (good for letting it run on its
  own, like on a build server)
- `--verbose`
  → "Show me everything you are doing on the screen." (normally the robot works
  quietly and only writes to log files)
- `--no-build`
  → "The app is **already** built — don't build it again. Just build and run
  the test." If the app turns out **not** to be built, the test will fail, and
  that's on purpose: it's better to fail loudly than to quietly rebuild.

So all together: *"Run this one test, quietly-to-me-no, loudly yes, don't waste
time rebuilding the app, and never pause to ask me a question."*

### The review shortcut: `--test-review`

Typing all those flags every time is annoying. So there is a shortcut:

```sh
./build.sh --test-review tests/auto/tools/qmakelib/qmakelib.pro
```

This **one** flag means the same as:

```sh
./build.sh --test tests/auto/tools/qmakelib/qmakelib.pro --headless --verbose --no-build
```

It is made for quickly **reviewing** a test against an app you already built.

You can change its mind with the opposite flags (in any order):

| Add this flag           | And the shortcut does this instead          |
| ----------------------- | ------------------------------------------- |
| `--build`               | build the app first (not `--no-build`)      |
| `--headed` or `--interactive` | allow questions (not `--headless`)    |
| `--no-verbose`          | work quietly (not `--verbose`)              |

Example — review, but build first and stay quiet:

```sh
./build.sh --test-review tests/auto/tools/qmakelib/qmakelib.pro --build --no-verbose
```

### Finding a test by glob

Instead of the full path, you can give `--test` or `--test-review` a
glob and let the robot find the `.pro` for you:

```sh
./build.sh --test-review '**/*qmakelib*'
```

How it works and what to know:

- Every `.pro` under the project is searched, so the glob need not end in
  `.pro` — `**/*qmakelib*` finds `…/qmakelib/qmakelib.pro`.
- Only **real test projects** count — ones that say `CONFIG += testcase`
  (the same mark the test runner looks for). Dummy `.pro` files that a
  test feeds to qmake as fixtures (for example the ones under a test's
  `testdata/` folder) are ignored, so a loose glob will not accidentally
  pick one.
- The glob is matched against each file's path **relative to the project
  root**, where `*` (and `**`) also reach across folder separators.
- **Every matching test runs** — like a test-pattern in Jest. No prompt,
  no "narrow it down". If the glob matches several tests, each one is
  built and run in turn (each in its own build and log folder so they
  don't clobber each other), and a short tally at the end says how many,
  if any, failed. If the glob matches no test, the robot stops with an
  error.
- **Quote the glob** (`'**/*qmake*'`). If you leave it unquoted, your own
  shell may expand it first and the robot never sees the pattern.

### Run the app after building

```sh
./build.sh --run
```

Builds the app, then launches it. (For a multi-part project like XD, you also
say *which* part: `./build.sh --run some/subdir`.)

### Clean up

```sh
./build.sh --clean        # remove build leftovers
./build.sh --wipe         # delete the whole build folder and start fresh
```

---

## Part 2 — The details

### The steps the robot follows (in order)

Every run goes through these steps. Some steps only happen if you asked for
them.

1. **Wipe** *(only with `--wipe`)* — throw away the whole build folder.
2. **Clean** *(only with `--clean`)* — remove leftover build files.
3. **qmake** — turn the project files (`.pro`, `.pri`, `.prf`) into build plans
   (called *Makefiles*). This is like writing a recipe before cooking.
4. **Build** — actually compile the code into a working program. *(skipped with
   `--no-build`)*
5. **Test** *(only with `--test`/`--test-review`)* — build the test program and
   run it.
6. **Run** *(only with `--run`)* — launch the program.

### Every flag

| Flag | What it means |
| ---- | ------------- |
| `--debug` | Build the easy-to-inspect version. **This is the default.** |
| `--release` | Build the fast version. |
| `--test [file.pro \| glob]` / `--tests` | Build, then build and run the tests. You may name one test `.pro` file, or a glob; a glob runs **every** test it matches (e.g. `'**/*qmake*'`). Without one, the project's default tests run. See "Finding a test by glob" below. |
| `--test-review <file.pro \| glob>` | Shortcut for `--test <file.pro> --headless --verbose --no-build`. Takes a `.pro` path or a glob, same as `--test`. Each of those three defaults can be flipped (see the table above). |
| `--run [subdir]` | Run the built program. A multi-part project needs the `subdir` to say which one. |
| `--build` | Force a build. (Used to cancel `--test-review`'s built-in `--no-build`.) |
| `--no-build` | Don't build — assume it's already done. With `--test`/`--run`, those still happen; if the build wasn't really done, they fail. |
| `--headless` | Never pause to ask a question. Good for unattended runs. |
| `--headed` / `--interactive` | The opposite: questions are allowed. |
| `--verbose` / `-v` | Show all the work on the screen (not just in log files). |
| `--no-verbose` | Stay quiet. |
| `--no-progress` | Hide the moving progress bar. |
| `--clean` | Remove build leftovers (in XD, also the auto-made `.pri` files) to act fresh. |
| `--wipe` | Delete the entire build folder. |
| `--only-clean` | The same as `--clean --no-build` together. |
| `--qmake` / `--refresh` | Only redo the build plans (Makefiles); don't compile. |
| `--ignore-pri` | When deciding if the plans are out of date, look only at `.pro` files and ignore `.pri` file times. Handy when an editor keeps "touching" a `.pri`. |
| `-d <dir>` / `--directory <dir>` | Build into a different folder than the usual one. A relative folder is measured from where you run the command; an absolute one (including a Windows path like `C:\…`) is kept as-is. |

### Smart shortcuts the robot already knows

- It **skips qmake** when the plans are still newer than every project file, so
  it doesn't redo work for nothing. `--qmake` forces it anyway.
- `--no-build` plus `--test` (or `--run`) still does the test/run part — it only
  skips rebuilding the main app.
- `--test-review` turns on its three defaults **first**, then your other flags
  are read normally — so any of them overrides a default, even a flag you wrote
  **before** `--test-review`. (For example, `--no-verbose --test-review …` stays
  quiet.) The defaults reuse the same on/off settings as the normal flags, so no
  special extra settings are involved.

### Two kinds of projects

The robot can handle two shapes of project:

- **One program** (called the *app* shape) — like a single application.
  The helper `bh_template_app` handles this.
- **Many programs** (called the *subdirs* shape) — a project made of several
  smaller projects, like XD itself and its big tests folder.
  The helper `bh_template_subdirs` handles this.

A project's `build.sh` picks the right shape for you; you don't have to.

### A note on tests and `target_wrapper.sh`

When tests run with `make check`, the recipe calls a tiny generated script
called `target_wrapper.sh` (it just sets a couple of paths, then runs the test).
That script lives right next to the test, so the recipe must call it as
`./target_wrapper.sh` — with the `./` — or the system won't find it. The XD
build sets this up correctly (see `mkspecs/features/qt_functions.prf`).

### Settings you can set ahead of time (advanced)

Before calling the robot, a `build.sh` (or your shell) can set some `BH_*`
values to change behavior. A few useful ones:

- `BH_ROOT` — the project's main folder (required).
- `BH_TEST_ROOT` — which test project to build for `--test` (a `.pro` file
  given after `--test` overrides it).
- `BH_BUILD_DIR_SUFFIX` — add a suffix to the build folder name (XD uses
  `-static` for its static build).
- `BH_VERBOSE` — same as `--verbose` if set to something other than `0`.

That's it. Start with **Part 1**, and reach for **Part 2** only when you want to
know exactly what a flag does.

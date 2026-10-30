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
| `--debug` \
| Build only the easy-to-inspect version. **This is the default** for an app. \
  Without a flag, a project made of sub-projects, such as XD, builds the \
  versions its settings name: for XD, both versions on Windows and macOS, and \
  only the fast one on Linux (since both versions would share one file name). |
| `--release` | Build only the fast version. |
| `--test [file.pro \| glob]` / `--tests` | Build, then build and run the tests. You may name one test `.pro` file, or a glob; a glob runs **every** test it matches (e.g. `'**/*qmake*'`). Without one, the project's default tests run. See "Finding a test by glob" below. |
| `--test-review <file.pro \| glob>` | Shortcut for `--test <file.pro> --headless --verbose --no-build`. Takes a `.pro` path or a glob, same as `--test`. Each of those three defaults can be flipped (see the table above). |
| `--run [subdir]` | Run the built program. A multi-part project needs the `subdir` to say which one. |
| `--build` | Force a build. (Used to cancel `--test-review`'s built-in `--no-build`.) |
| `--no-build` | Don't build — assume it's already done. With `--test`/`--run`, those still happen; if the build wasn't really done, they fail. |
| `--headless` | Never pause to ask a question. Good for unattended runs. |
| `--headed` / `--interactive` | The opposite: questions are allowed. |
| `--verbose` / `-v` | Show all the work on the screen (not just in log files). |
| `-q` / `--quiet` | Show errors only (no progress bar). |
| `-vv` / `--very-verbose` | Show even more: also qmake's "Reading …" lines. |
| `--no-verbose` | Stay quiet. |
| `--no-progress` | Hide the moving progress bar. |
| `--clean` | Remove build leftovers (in XD, also the auto-made `.pri` files) to act fresh. |
| `--wipe` | Delete the entire build folder. |
| `--only-clean` | The same as `--clean --no-build` together. |
| `--qmake` / `--refresh` | Only redo the build plans (Makefiles); don't compile. |
| `--ignore-pri` | When deciding if the plans are out of date, look only at `.pro` files and ignore `.pri` file times. Handy when an editor keeps "touching" a `.pri`. |
| `--release-debug` | Build the fast version, but keep the debug info in it. |
| `--force-qmake` | Like `--qmake`, and when qmake fails for one project, it goes on with the others (the failure is still reported). |
| `--save-space` | Save disk while building: each library or program drops its build leftovers (object files, moc/rcc output) right after it links, and a project that finished once is skipped. |
| `--cross-compile[=<spec>]` / `--cross-build` | Build for another platform (`win32-g++` without a value), while the build tools stay on this one. |
| `--filter=<regexp>` / `--only=<regexp>` | Build only the projects whose path matches the regexp (`grep -E`), one at a time; `a\|b` names several. |
| `--rebuild[=<regexp>]` | Build the matching projects again (every project without a value): without `--save-space` each is cleaned first, with it each builds even when it finished once. With `--filter`, only among the projects `--filter` keeps. |
| `--build-tests` | Make the `--test` step build each test again. |
| `--no-build-tests` | Make the `--test` step only run the tests built before (by default it follows `--build` / `--no-build`). |
| `--static[=<regexp>]` \
| Build in *static* mode (`qt_static`): the program carries the framework \
  inside it instead of needing its library files next to it. Without a value \
  every project builds static; with one, only the projects whose path matches \
  the regexp (`grep -E`). With `--filter`, it only picks among the projects \
  `--filter` keeps, just like `--rebuild`. When a new regexp moves a project \
  in or out of static mode, that project is cleaned first; hence nothing \
  compiled for the other mode is left. It builds into its own folder, with \
  `-static` before the mode (for example `jom-static-debug`); hence the normal \
  build stays as it is. When any project builds static, a project built on XD \
  also gets XD built in static mode first (XD's own `build.sh --static`, given \
  every flag and qmake argument of this run, except the ones that name this \
  project's own paths or steps: `--filter`, the `--static` regexp, `--test`, \
  `--run`, and `-d`), unless XD already has a whole static build. |
| `--no-static` \
| Build every project in the normal (shared) mode again, even if the project's \
  `build.sh` turned `--static` on by default. |
| `--xd-<flag>` / `--no-xd-<flag>` (or `--qt-` / `--no-qt-`) \
| Forwards `--<flag>` (or `--no-<flag>`) to the XD builds this run starts. A \
  value needs `=`: `--xd-rebuild=<regexp>` forwards `--rebuild=<regexp>`, while \
  a `--xd-rebuild <regexp>` without `=` sign forwards a bare `--rebuild` and \
  leaves `<regexp>` to this run, which may break this run, meaning `<regexp>` \
  is never passed to XD's build if the `=` sign is missing. |
| `-d <dir>` / `--directory <dir>` | Build into a different folder than the usual one. A relative folder is measured from where you run the command; an absolute one (including a Windows path like `C:\…`) is kept as-is. |

### Internal flags

A run passes these to the builds it starts itself; you never need to type them.

| Flag | What it means |
| ---- | ------------- |
| `--child` | Marks a sub-run, meaning a run that another run started (each dependency build gets it). Only if this is passed does the sub-run not ignore special environment variables, like `BH_XD_BUILD_FILTER`, which narrows `--filter`, and `BH_BUILD_DIR_SHOWN`, which may skip the build folder prompt. |

### Smart shortcuts the robot already knows

- It **skips qmake** when the plans are still newer than every file qmake read,
  so it doesn't redo work for nothing. `--qmake` forces it anyway.
- It **redoes qmake** when `--static=<regexp>` picks other projects than when
  the plans were made; hence the new choice always takes effect.
- It **remembers the build order**: a project's first qmake records its place
  in that order when it has none yet (one the root project does not list, say;
  `--save-space` records every project anyway). Hence `--filter` builds its
  matches in their right order, whatever filter a later run uses.
- It **cleans** when a build folder was made with another toolchain (another
  spec, or `--memory-safe` turned on or off), right after it redoes qmake;
  hence nothing the old compiler built ends up in the new link.
- It **builds XD first** when a project built on XD finds XD's build
  incomplete: XD's own `build.sh` runs with this run's flags, except the ones
  that name this project's own paths or steps. `--no-xd-build` skips that.
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
- `BH_STATIC`: same as `--static=<regexp>`, which lets a project build
  static by default (`BH_STATIC='^.*$'` for every project); `--no-static`
  still turns it off.
- `BH_PROGRESS_MODE`: how the compile step shows its progress, from a count
  of the build actions (the build plan):
  - `precise` (the default) counts the whole plan before the build starts.
  - `estimate` shows a bar of `BH_ESTIMATED_ACTIONS` build actions at once,
    while the plan counts in the background, with no row of its own; the bar
    then takes the plan's count. `--estimate-actions=<count>` picks this mode
    with that estimate.
  - `spinner` skips the plan, and only spins.

  A build that ends well notes how many build actions it performed, and how
  long they took; the note also gives the estimate, unless `--filter` or
  `--rebuild` narrowed the build.
- `BH_XD_BUILD_FILTER`: a regexp that narrows the XD builds the run starts
  (`bh_xd_build` and `bh_xd_build_static`) to the projects it matches; a
  `build.sh` sets it after loading `build-handler.sh`. Those builds then run
  on every run, doing only what changed, as the run cannot tell whether XD
  has those parts already; `--xd-filter` only narrows it further.

Two variables are special, as a run passes them on to the builds it starts
(see `bh_dependency_build`), and only a `--child` run takes them from its
environment; any other run ignores what it inherited:

- `BH_XD_BUILD_FILTER` reaches XD's builds, whose `--filter` it narrows.
- `BH_BUILD_DIR_SHOWN` is the build folder the run showed and asked about
  (see `bh_build_dir_gate`); a build it starts does not show that folder
  again. The run sets it itself.

A `build.sh` can also run its own code once the flags are read, while that
code can still change what the flags set:

```sh
bh_hook args-parsed <<'}'
    bh_filter_add "$my_regexp"
}
```

The code between the first line and the lone `}` runs right after the flags
are read. Here, `bh_filter_add` keeps only the projects that both `--filter`
and `my_regexp` match: it adds the regexp to the filter as one more line, and
a filter of several lines keeps only what every line matches. XD's `build.sh`
uses this to turn its `--static=<regexp>` into a filter.

A `build-deps` hook runs later: when the run builds, right before the first
qmake. That is the place to build the other frameworks a project needs.
`bh_dependency_build` builds one through its own `build.sh`, with this run's
flags; `bh_dependency_build_static` does the same in static mode, and only
when some project builds static:

```sh
bh_hook build-deps <<'}'
    bh_dependency_build "$other_framework/build.sh"
    bh_dependency_build_static "$other_framework/build.sh"
}
```

That other build gets this run's build settings (such as `--release`,
`--cross-compile` and the qmake arguments), but no flag that names this
project's own paths or steps (`--filter`, the `--static` regexp, `--test`,
`--run`, `-d`), nor one that would redo or drop what the framework
built (`--clean`, `--rebuild`, `--wipe`, `--qmake`, `--refresh`,
`--force-qmake`): a framework such as XD is no small part of the build, hence
`--xd-filter=<regexp>`, `--xd-clean`, `--xd-rebuild=<regexp>` and the like ask
XD's build for them. `--only-clean` passes on only its `--no-build` part.
It also always gets `--headless`, since the run asks everything it needs up
front; nothing stops halfway through to ask.

A project that lives inside the XD tree can build in XD's own build folder,
at its own path there: `bh_xd_module_build_dir` gives that folder, from the
project's path inside `XD_DIR` (`src/app` builds in `../build/xd/src/app`, or
in `../build/xd-static/src/app` in static mode). `bh_xd_build_dir [<subdir>]`
gives XD's build folder itself, with `<subdir>` below it when given; XD's own
`build.sh --static` uses it too.

Such a project can build just the XD parts it needs: `BH_XD_BUILD_FILTER`
narrows the XD build the run starts to the projects it matches, which
`--xd-filter` only narrows further. That build then runs on every run, doing
only what changed. And with
`bh_hook_add display-build-dir bh_xd_build_dir`, the build folder the run shows
(and asks about) is XD's, which holds the project's. A `build.sh` that does
both:

```sh
BH_XD_BUILD_FILTER='^src/(corelib/corelib|tools/moc/moc)\.pro$'
bh_hook args-parsed <<'}'
    BUILD_DIR=$(bh_xd_module_build_dir) || exit 1
}
bh_hook_add display-build-dir bh_xd_build_dir
```

A `test-env` hook runs right before each test that `--test` runs; it is the
place to export what the test binaries read from their environment.

A function the `build.sh` already defined can be a hook too: `bh_hook_add`
adds a call of it to a stage, and the call runs just like the code `bh_hook`
adds:

```sh
my_prepare_tests() {
    MY_TEST_SETTING=on
    export MY_TEST_SETTING
}
bh_hook_add test-env my_prepare_tests
```

A run reaches these hook stages in this order:

- `args-parsed`: right after the flags are read, while a hook can still change
  what they set (`BH_FILTER`, `BH_MODE`, and the like; see `bh_filter_add`).
- `display-build-dir`: prints the build folder the run shows and asks about;
  `bh_build_dir`, which prints `BUILD_DIR`, comes first, and the last hook's
  output is the folder shown.
- `build-deps`: when the run builds, right after XD's own builds and before
  any qmake; a hook builds the other frameworks the project needs (see
  `bh_dependency_build`).
- `test-env`: right before each test runs; a hook exports what the test
  binaries read from their environment.

The helpers below are documented here rather than in `build-handler.sh`,
whose own comments stay short: every run, and every sub-shell build it starts,
reads that whole file.

### Build folder helpers

- `bh_build_dir_gate` shows the build folder and asks whether to go on with it
  (see `bh_license_gate`). The folder shown is the result of the
  `display-build-dir` hooks: `BUILD_DIR` (see `bh_build_dir`) unless a later
  hook prints another.
- `bh_build_dir_root <dir>` finds and prints the out-of-repo `build`
  directory's path, which contains the actual per-project build directories.
  For the tree at `<dir>`, it sits beside `<dir>`, or, when `<dir>` is inside a
  git submodule, beside the outermost repo of that submodule chain, as a build
  folder inside a repo would show in its `git status`. When that repo is the
  root of the file system (`/`, or a drive such as `C:/`), the `build` folder
  goes right below that root, as it has nowhere else to go.

### Progress helper

- `bh_progress_note <line>` prints `<line>` above the progress row: it clears
  the row, prints `<line>` where it was, then draws the row again below it;
  hence the row stays pinned under the lines a step prints. Without a live
  progress display, it only prints `<line>` to stderr.

### XD build helper

- `bh_xd_dependency_build [argument...]` builds XD through
  `bh_dependency_build` with the arguments, and with the `--xd-<flag>` flags of
  this run (`BH_XD_ARGS`) as `--<flag>`, after this run's other flags (hence
  they win) and ahead of the qmake arguments; that build also gets
  `BH_XD_BUILD_FILTER` (see `bh_xd_build`).

### Build order helper

- `bh_build_unrecorded [mode-tag]` builds the projects that the build order
  does not list. If the mode-tag is set to "count" without quotes, then it only
  prints how many projects it would build, hence the counter of each line can
  cover the recorded projects and these at once.
- `bh_filter_paths [--relax]` passes on, from the absolute `.pro` paths on its
  input, the ones that `--filter` can keep (or, without `--filter`,
  `--rebuild`), with one `grep -E` per line of its regexp for all of them at
  once; hence a loop asks `bh_filter_candidate` about the few that pass, not
  about every `.pro` of the tree. It drops a path that does not start with
  `"$BH_ROOT/"`, unless the caller passes `--relax`, which skips that check;
  pass it only when every path is sure to start with `"$BH_ROOT/"` (as the
  paths of `bh_pro_files` do, since `find` prints them under `BH_ROOT`).
- `bh_grep_lines <regexp>` passes on the input lines that every line of
  `<regexp>` matches (see `bh_regexp_matches`); finding no line is no failure.

### Clean helpers

- `bh_make_clean <makefile> [<log>]` runs the `clean` target of `<makefile>`,
  so the next make builds every object of that project again; it does nothing
  when `<makefile>` does not exist yet. GNU make gets `-o <makefile>`, so it
  never runs qmake first through the Makefile's own rule, with the old
  Makefile's command line; the qmake step decides about qmake (see
  `bh_run_qmake`). Without `<log>` it is a `clean` step of its own; with
  `<log>`, it only adds its output to `<log>` and returns make's exit code, as
  a part of a step the caller shows (see `bh_clean_candidates`).
- `bh_qmake_clean <makefile> <label>` cleans what the last build of
  `<makefile>` made, as the end of the qmake step that `bh_run_qmake` runs for
  it: on that step's row and number, with a log of its own
  (`<step>-qmake-clean.log`).
- `bh_clean_candidates`, which cleans the projects `--clean` names under
  `--filter` and/or `--rebuild`, makes all of them one `clean` step, with one
  log and one spinner row; the `make clean for <project>` line of each goes
  above that row as it starts (see `bh_progress_note`). Each clean runs to its
  end even on Ctrl+C, and the build stops right after (see `bh_held_start`),
  as the spinner turns in the foreground meanwhile.

That's it. Start with **Part 1**, and reach for **Part 2** only when you want to
know exactly what a flag does.

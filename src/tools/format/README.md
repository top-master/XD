# format

Applies the formatting rules to C and C++ files, all at once: it fixes each
break a rule can fix safely, and lists the ones a person has to fix.

Run it on the code you added or changed: on a file you wrote, or with `-L` on
the lines you edited in an existing file, so that the rest of the file keeps
the layout it has.

## Usage

```
format [options] [paths...]
format --list-rules
```

- `paths`: the files to format. A path that ends with `/` or `\` is a
  directory instead, searched for C and C++ files (see "Directories" below).
- `-L <range>`, `--range <range>`: formats only the lines of `<range>`, in any
  form that `git blame -L` takes (see "Line ranges" below). Give it more than
  once for more ranges; without it, the whole file is formatted.
- `--dir <path>`: formats the C and C++ files under the directory `<path>`,
  with or without a trailing slash. Give it more than once for more
  directories.
- `--check`: writes nothing; only tells which files need formatting.
- `--rules <names>`: runs only the rules of `<names>`, separated by commas.
- `--list-rules`: lists each rule's name and what it checks, then exits; the
  names are the ones `--rules` and the issues use.
- `-j <count>`, `--jobs <count>`: formats up to `<count>` files at once; the
  default is one file per CPU core.
- `--runner <kind>`: spreads the files with `thread` (the default), `process`
  (child processes of the tool), or `serial` (one file after the other). A
  build without threads has no `thread` kind, and one without processes has no
  `process` kind; `--help` lists the kinds of the build.
- `-c <setting>`, `--config <setting>`: sets `<setting>`, written as
  `<section>.<key>=<value>` (such as `-c=types.refsuffix=none`), over the
  `.formatrc` files; section and key may be in any case. Give it more than
  once for more settings (see "Settings" below).
- `--config-file <file>`: reads the settings of `<file>` in place of every
  `.formatrc` file.
- `--config-add <file>`: reads the settings of `<file>` over the `.formatrc`
  files; it overrides only the keys it sets. Give it more than once for more
  files.
- `--any-comment`: wraps again every comment paragraph that leaves room
  unused, not only the ones past `wrapMax`; the same as
  `-c comments.target=any` (see "Wrapping comments" below).
- `-I <dir>`, `-i <dir>`: looks for the headers that quoted `#include`s name
  in `<dir>` too, after the file's own folder, the way a compiler does; give
  it more than once for more folders. A rule that reads a class from its
  header (`outline-this`) then finds a header kept in an include folder.
- `-p <file>`, `--project <file>`: formats the files that the qmake project
  `<file>` lists, as qmake reads it: the tool runs `qmake -E <file>` in a
  scratch folder (it writes no Makefile there), and reads `SOURCES`,
  `HEADERS`, `OBJECTIVE_SOURCES`, and the include folders of `INCLUDEPATH`. A
  `subdirs` project formats the projects of its `SUBDIRS` instead (through
  `<name>.file` or `<name>.subdir` when it sets them). A project formats only
  its own files: the ones its `STATIC_PLUGINS` bring in belong to the
  plugin's own project, which lists them in `PLUGIN_SOURCES` and
  `PLUGIN_HEADERS`. It skips, without a word, the files that the ignore files
  leave out (such as `src/3rdparty`). Give it more than once for more
  projects. A file that only a scope of another platform lists
  (`win32: SOURCES += ...`) counts only under that platform's spec (see
  `--specs`). `build.sh --format` runs the tool this way before it builds: on
  the build's own project, or with `--filter` on each project the filter
  picks; when it cross-compiles, for the host spec and the target spec.
- `--specs <names>`: reads each `--project` for each qmake spec of the
  comma-separated `<names>` (such as `linux-g++,win32-g++`), which pick the
  platform scopes; give it more than once for more specs. The first spec is
  read before any file is formatted, and a thread reads the others while the
  files found so far are formatted; a file that several specs list is
  formatted once. Without it, the spec of the platform and compiler the tool
  was built for (such as `linux-clang`).
- `--qmake <path>`: reads each `--project` with the qmake at `<path>`; by
  default the one beside the tool, else the first `qmake` on the `PATH`.
- `--skip-ignored`: skips each named path that the ignore files leave out,
  without a word (instead of the warning and the question, or of formatting
  it with `--headless`); with nothing left, the tool exits clean.
- `--ai`: to learn more about this option, read
  [`assets/ai-start.md`](./assets/ai-start.md).
- `--headless`: asks nothing. Without it, the tool warns about each named
  path that the ignore files leave out (naming the rule), and asks whether to
  go on; an answer of `n` or `no` stops the run, any other answer (or an input
  that ended) goes on.
- `--worker`: prints the results in the form a parent `format` process reads;
  the `process` runner passes it to its children.

## Directories

A directory is searched with all of its subdirectories, for the files ending
in `.h`, `.hh`, `.hpp`, `.hxx`, `.c`, `.cc`, `.cpp`, `.cxx`, `.m`, or `.mm`.

The search leaves out what the project's ignore files leave out, the way git
does:

- The project's root is the folder that holds the closest `.git` (a folder,
  or a file as in a worktree) above the searched path; without one, the
  searched folder itself.
- Each folder from the root down may hold a `.gitignore` and a
  `.formatignore`. They are read in that order, from the root down; the last
  rule that matches a path decides. Hence a folder's files override the ones
  of the folders above it, and a `.formatignore` overrides the `.gitignore`
  beside it. Both take git's patterns, such as `!/<path>` to keep a path that
  `.gitignore` leaves out.
- The `.git` folder is always left out.
- A path named on the command line (a file, or a folder with or without
  `--dir`) is never left out, and nor is a folder on the way to it, as if a
  `!/<path>` rule followed all the others. The files inside a named folder
  still follow the rules.

A path given without a trailing slash is always a file. When it names a
directory, the tool stops with an error; add the slash, or pass the path with
`--dir`.

## Settings

A project may set choices the rules leave open, in `.formatrc` files (in the
`.ini` form, with sections and keys in any case). Just like git reads its
`.gitignore` files, the tool reads each `.formatrc` from the project's root
(the folder of the closest `.git`) down to the file's own folder: for
`a/b/c/my.cpp`, first the `.formatrc` of the root, then `a/.formatrc`, and so
on until `a/b/c/.formatrc`. A deeper file overrides the keys it sets again.

```
[Types]
refSuffix = keep-as-is
moveSuffix = " "
```

- `refSuffix`: the spacing after the `&` of a named reference declaration:
  any run of spaces and/or tabs to write there, such as `" "` (one space,
  `Type & name`; the default), `"  "` (two), `"\t"` (a tab), or `""` or
  `none` (no space, `Type &name`); `keep-as-is` (the spacing there stays); or
  a replacement by regular expression (see below). The space before the `&`
  is always one.
- `moveSuffix`: the same for the `&&` of a named rvalue reference
  declaration; the default is `keep-as-is`.
- `wrapBegin` (in `[Structure]`): the column a long line wraps at, or
  several joined by `|`, tried in order; the default is `75`. A long
  condition, conditional expression, or call breaks at the first of them (see
  "Laying out code" below).
- `wrapMax` (in `[Structure]`): the column no line may pass, for the code
  lines that the parameter, call, operator, and `connect()` rules wrap; the
  default is `80`.
- `wrapBegin` and `wrapMax` in `[Comments]`: the same for comments (see
  "Wrapping comments" below). `[Comments]` takes every key of `[Structure]`
  whose name starts with `wrap` (`wrapArgsAt` below too, which no comment
  rule reads yet), and each one follows its `[Structure]` key, default
  included, until a `.formatrc` sets it in `[Comments]`:

  ```
  [Structure]
  ; Code and comments wrap at 100.
  wrapMax = 100
  [Comments]
  ; Comments wrap at 90 instead.
  wrapMax = 90
  ```
- `wrapExtendsForKeywords` and `wrapEmptyLimit` (in `[Comments]`): how many
  columns past `wrapMax` a comment line may run to keep a code keyword at its
  end, and how many columns the break before that keyword must leave free at
  least; the defaults are `20` and `8`.
- `wrapComma` (in `[Comments]`): `soft` (the default) lets a comment line
  break after a word's `,` only within `wrapEmptyLimit` columns of the wrap
  column; `hard` lets it break there within the 25 columns of the other
  punctuation. A `),` counts as its `)` either way.
- `target` (in `[Comments]`): `long` (the default) wraps again only a comment
  paragraph with a line past `wrapMax`; `any` also wraps one whose lines leave
  room unused (see "Wrapping comments" below). `--any-comment` sets `any`.
- `thisMode` (in `[Relations]`): where a member function reaches a member
  of its class, or of a base class, through `this->` (the rule
  `outline-this` adds the `this->` there, and drops it elsewhere): a
  condition of names joined by `|` (or) and `&` (and, which binds closer),
  with parentheses for grouping. The names hold for a member name:
  - `always` everywhere, and `never` nowhere;
  - `already-existing` where the code already says `this->`;
  - `is-base-member` for a base class's member;
  - `is-template` in a template, or a member of a class template;
  - `is-outline` in a body written outside the class body
    (`void Widget::run() { ... }`);
  - `is-header` in a header (`.h`, `.hh`, `.hpp`, `.hxx`, `.h++`, or a file
    without a suffix), and `is-non-header` in any other file.

  The default, `is-base-member | already-existing | (is-outline & is-header)`,
  keeps each `this->` there is, and adds one for a base class's member, and
  for every member in an outline body of a header. Other choices, each one a
  `thisMode` line of its own:

  ```
  [Relations]
  ; Every member, everywhere.
  thisMode = always
  ; No new `this->`, and none dropped.
  thisMode = never | already-existing
  ; Only the base members of templates (found through `this->` alone).
  thisMode = (is-base-member & is-template) | already-existing
  ```
- `wrapChainAt` (in `[Structure]`): where an operator chain that does not
  fit one line breaks before its operators (see `operator-wrap` under
  "Laying out code" below): a condition of the same names, and of percents
  from `0%` to `100%`. A percent holds when the column where an operator
  starts (right after what comes before it) is past that share of
  `wrapMax`; the chain's leading operators that start before it stay on its
  first line, while that line fits, and each operator from the first one
  past it gets a line of its own. The default is `40%`; `0%` breaks before
  every operator, and `100%` only where the line would not fit.
- `wrapArgsAt` (in `[Structure]`): where a call whose arguments do not fit
  one line breaks after its `(` (see `args-wrap` under "Laying out code"
  below): a condition of the same names, and of percents from `0%` to
  `100%`. A percent holds when the column right after the `(` is past that
  share of `wrapMax`; with `wrapMax = 80`, `40%` holds from column 33 on.
  The default is `40%`; a name alone needs no percent:

  ```
  [Structure]
  ; Breaks sooner in a header.
  wrapArgsAt = (40% & is-non-header) | (30% & is-header)
  ; Breaks every wrapped call of a source file.
  wrapArgsAt = is-non-header
  ```

The command line then overrides the `.formatrc` files, in this order:

1. `--config-file <file>`, when given, is read in place of every `.formatrc`.
2. Each `--config-add <file>`, in the order given, overrides the keys it sets.
3. Each `-c <section>.<key>=<value>`, in the order given. A value in double
   quotes is read as a C++ string literal, with every escape one takes
   (`-c 'types.refSuffix="\t"'` for a tab, `\x20`, `\u00a0`, and so on); a
   value without quotes stays as it is, backslashes included.

### Replacements by regular expression

A spacing key also takes `regexp/<pattern>/<replacement>/`. On each line where
the rule looks up that key (for `refSuffix`, a line with a named reference
declaration), the tool replaces every match of the pattern with the
replacement, once per run. In the replacement, `\0` stands for the whole
match, `\1` to `\99` for the pattern's groups (for `(abc.*)(xyz.*)`, `\2\1`
swaps them), and `\\` for one backslash; a group the pattern lacks is an
error. A `\/` stands for a `/` in either part.

```
[Types]
refSuffix = regexp/[^&]&\\s*/&/ | keep-as-is
```

In a `.formatrc`, a backslash starts an escape of the `.ini` form (`\t` is a
tab, and an unknown one such as `\s` is dropped whole); hence each backslash
of a pattern or a replacement is written twice there, as above. On the
command line, a value without quotes needs no doubling:
`-c 'types.refSuffix=regexp/[^&]&\s*/&/ | keep-as-is'`.

- A `| <value>` after it names the choice for a line the pattern matches
  nothing on, such as `| keep-as-is` or `| " "`; without one, that line gets
  the key's default. The value after `|` cannot be a `regexp/` itself.
- A line the replacement took is left to it: the rule changes nothing else
  on that line, even where the result no longer matches the pattern.
- The replacement runs once per run, not until nothing matches; hence a
  pattern that matches its own result runs again on the next run of the
  tool. Write a replacement whose result the pattern no longer matches.

A section, a key, or a value the tool does not know is an error: on the
command line, before any file is formatted; in a `.formatrc`, for each file
it applies to.

## Wrapping comments

A paragraph of a comment with a line past `wrapMax` is wrapped again, from
its first line on. With `target = any` (or `--any-comment`), so is one with a
line that ends `wrapEmptyLimit` columns or more short of the wrap column while
the next line's first word would fit on it; the default, `target = long`,
leaves such a paragraph as it is, so that a diff touches only the comments
that are too long. Each line breaks at the space nearest the wrap column
(`wrapBegin`) that follows a `,`, `;`, `)`, or `.`, or that comes before a
`(`, within 25 columns before the wrap column (for a word's `,`, see
`wrapComma`) and not past `wrapMax`. Else a word that points to what follows
(`a`, `an`, `the`, `and`, `or`, `but`, `nor`, `where`, `which`, `that`,
`when`, `while`, `whose`, `whom`) ends the line, when it ends within
`wrapEmptyLimit` columns before the wrap column; else the line breaks at the
last plain space up to the wrap column, or, when a word reaches past it, at
the first one within `wrapMax`. No
break falls inside backtick code, and a last line of 17 characters or fewer
takes words from the line before it.

A code keyword that the break would push to the next line stays on the line
instead, when the break would leave at least `wrapEmptyLimit` columns free
before the wrap column; the line may then pass `wrapMax` by up to
`wrapExtendsForKeywords` columns. A code keyword is a name in backticks, or a
word with `::`, `->`, `()`, an underscore, or an inner capital (such as
`QSslConfiguration::setSessionTicketKey`). Such a line counts as fitting; it
is not reported or wrapped again.

With `wrapBegin = 75 | 51`, the paragraph wraps at 75 first; when its last line
fills less than half of that width, it wraps at 51 instead, and so on down
the list. The first run whose last line fills at least half its width wins;
when none does, the run whose last line fills the most.

A paragraph ends at a blank line, a fenced block, a command line such as
`\class`, a separator line (a rule of dashes), or a list item; a list item's
own lines go on under its text. A line that ends with a hyphen after a letter
joins the next one without a space (`session-` and `ticket` give
`session-ticket`). A comment at the end of a line of code is not wrapped, and
a line that no break brings within `wrapMax` is reported.

## Laying out code

The code rules lay out a few constructs on their own:

- `not-space` puts a space on each side of a logical not (`if (!ready)`
  becomes `if ( ! ready)`); a run of signs stays together (`!! x`). It leaves
  `!=`, `operator!`, preprocessor lines, and a side that has its space
  already.
- `control-braces` wraps each body of `if`, `else`, `for`, `while`, `do`, and
  `foreach` in braces. In a header, a body on its keyword's line stays there
  (`if (x) { return; }`); in any other file it moves to a line of its own,
  one step (4 spaces) past the keyword's line, with its `}` on the next line,
  also when it had its braces already.
- `condition-wrap` breaks an `if` or `while` condition that passes the wrap
  column before each `&&` and `||`, at each level of parentheses; each
  operand line starts with its operator, one step past the line that opened
  its group. A group that holds a parenthesized operand closes its `)` on a
  line of its own, and so does the condition, at the keyword's indent, with
  the body's `{` after it.
- `args-wrap` breaks a call of 2 arguments or more after its `(` when the
  call, written on one line, would pass the wrap column, and it takes more
  than 3 arguments or `[Structure] wrapArgsAt` holds at its `(`: each
  argument on a line of its own, one step past the line of the `(`, and the
  `)` back at that line's indent. A call that fits one line keeps its
  layout, and so does a call already broken after its `(` whose lines fit
  `wrapMax` (such as one whose related arguments share a line):

  ```
  const QByteArray input = makeFile(
      "first",
      "second"
  );
  ```
- `ternary-wrap` breaks a statement with a conditional expression that passes
  the wrap column before its `?`, one step past the statement's line, then
  before its `:` too when that line still does not fit. `condition-wrap`
  breaks an operand the same way, one step past the operand's line:

  ```
  if ( ! myFirstCondition
      && (mySecond
          || (myThird
              && (ov = manager
                  ? manager->override()
                  : QSharedPointer<QHostInfoOverride >()
              )
          )
      )
  ) {
  ```

## Comments the rules leave alone

- a `TRACE/` record and a license header;
- the label of a closing line, such as `} // namespace` or
  `#endif // QT_NO_THREAD`, and a label between code on one line, such as a
  block comment that names an argument's parameter;
- a one-word comment, such as the text of a data row (`// ept-enco`), which
  keeps its case and gets no period;
- every comment of a data table that a comment saying `format: table`
  precedes: the braced initializer after that comment, up to its closing
  brace.

## Line ranges

A `<range>` takes the forms of `git blame -L`:

- `<start>,<end>`: the lines from `<start>` to `<end>`;
- `<start>,` and `<start>`: from `<start>` to the end of the file;
- `,<end>`: from the first line to `<end>`;
- `<start>,+<count>`: `<count>` lines from `<start>` on;
- `<start>,-<count>`: `<count>` lines up to `<start>`;
- `:<funcname>`: the function whose first line matches the regular expression
  `<funcname>`, up to the next function. A function's first line is one that
  starts with a letter, `_`, or `$`.

A `<start>` or `<end>` is a line number, or a `/<regex>/`: the first line that
matches the regular expression. The search starts after the range before it
(or at the first line for the first range); `^/<regex>/` always searches from
the first line.

Each range applies to every file of the run. A range that does not fit a file
(past its end, or a regex with no match) is an error for that file.

## Output and exit code

Each line names a file and what happened to it:

- `<path>: formatted`, or `<path>: needs formatting` with `--check`;
- `<path>:<line>: <rule>: <message>`, for a break a person has to fix;
- `<path>: error: <message>` (on the standard error), for a file the tool
  could not read, format, or write.

The exit code is `0` when no file needed a change and no break is left, `1`
when the tool changed a file (or would have, with `--check`) or a break is
left, and `2` on an error.

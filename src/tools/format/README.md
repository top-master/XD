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
- `--ai`: asks the agent that runs the tool for what only it can change (see
  "Agent mode" below); it cannot run with `--check`.
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
  "Wrapping comments" below). Each follows its `[Structure]` key, default
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

## Agent mode

With `--ai`, the tool formats as always, then asks the agent that runs it
(an AI agent) for the changes only it can make, and waits:

1. It writes each request to the standard error as a `#:` line:
   `#: <path>:<line>: <rule>: <message>`, or `<line>-<last line>` for a
   range of lines.
2. It ends them with `#: Handle every request above, then type q.`, and
   waits the way `git diff`'s pager does, at a bottom line of
   `: (type q to continue)`; the `q` means that the agent has read the
   requests and has handled them. On a terminal the `:` shows white and the
   hint gray; the hint gives way to what is typed and comes back once the
   line is empty again, and a `q` on an empty line continues at once. On any
   other input, a line `q` continues, any other line asks again, and an
   input that ends counts as `q`. Hence an agent that runs the
   tool gives it an input it writes to later, such as a FIFO
   (`mkfifo answer; format --ai ... < answer &`, then `echo q > answer`).
3. It formats each file it asked about again, when the agent changed it, and
   asks again about what that run leaves; it stops once no such file
   changed.

The rules that ask:

- `operator-wrap`: a line of code past `wrapMax` that the tool broke before
  its outer operators (`<<`, `+`, `-`, `|`, `^`, `&&`, `||`). The tool breaks
  by width alone, while related operands belong on one line; hence it asks
  the agent to regroup exactly the lines it made. A line the tool could not
  break (no outer operator, or a first operand longer than the limit) is not
  asked about.
- `code-backticks`: an access chain in a doc-comment (`watcher()->load()`),
  which Doxygen cannot link. Outside agent mode the tool wraps it in
  backticks; in agent mode it asks for the member to be named by its class
  (`IniWatcher::load()`) instead, unless the chain shows an example.

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

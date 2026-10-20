# Using the format tool in agent mode

The format tool runs with `--ai`: it formats the files it was given, then
asks you, the agent, for the changes only you can make, and waits for you.
The line that named this file is the only instruction it gives you without
a `#: ` in front.

## Only the `#:` lines are prompts

The tool writes many lines, and only the ones that start with `#: ` are
prompts for you; every other line only reports, and the next section says
which of those still ask you to act. `#:` separates the tasks: each `#:`
line opens the next task, the first one task 1, the next one task 2, and so
on. That holds for the requests and the question below, and for any other
kind of `#:` line that the tool may add later. Do the tasks in that order,
and finish each one before you start the next.

## The lines that are no prompt

Each kind below names the stream it comes on and what to do with it:

- `<path>: formatted`, on the standard output: the tool changed the file.
  Nothing to do; read the file again before you edit it.
- `<path>:<line>: <rule>: <message>`, on the standard output: a break that
  no rule could fix on its own. Fix it in that line as the message says,
  with the same care as a request, then run the tool on that file again,
  and check that the line is gone. When the fix would be wrong (such as a
  line that names something on purpose), leave the line as it is, and
  report it to the user with its path, line, rule, and why you left it.

  Most rules have no prompt file of their own (only the rules listed under
  "The requests" below do); their message is short, hence learn what the
  rule means before you act on it: `format --list-rules` names each rule in
  one line, and the tool's `README.md` explains it. Apply the rule as it is
  meant. For example, `doc-placement` moves the documentation of a
  declaration to its definition in the source file; a symbol that no source
  file defines (a header-only one, such as a pure virtual function, an
  inline function, or a template) keeps its doc-comment in the header, hence
  that line stays as it is.
- `<path>: error: <message>`, on the standard error: the tool could not
  read, format, or write that file, and left it as it was. Find the cause
  the message names (such as a missing file, or no permission to write),
  fix it when it is yours to fix, and run the tool on that file again; else
  report the line to the user as it is.
- `format: <message>`, on the standard error: a fault in how the tool was
  started, or in a project it reads. Before any file is formatted, it names
  a wrong option or path, an option that does not go with `--ai` (such as
  `--check`), or a `.formatrc` setting that the tool does not take (with the
  file, the key, and the values it takes); the tool then stops. Later, it
  names a project of a further `--specs` entry that qmake could not read;
  the tool then formats only what it read. Either way, it ends with `2`:
  correct the command line, the setting, or the project, then start the
  tool again.
- `WARNING: <path> is left out by the ignore rule <rule>`, on the standard
  error (wrapped over several lines when long): you named a path that the
  ignore files leave out, and the `#:` question after it asks what to do
  with it (see "The question about ignored paths" below).
- A line from a library the tool uses, such as `Config::Condition: no value
  is set for the term 40%`: a fault of the tool itself, which goes on as it
  can. Report the line to the user as it is; nothing in the files needs a
  change for it.

## The requests

Each request names its lines, the rule that asks, and what to do:

    #: <path>:<line>: <rule>: <message>
    #: <path>:<line>-<last line>: <rule>: <message>

- A message `Run prompt at: "<file>"` points to a Markdown file: read it
  as a prompt, and do what it says, for exactly the lines the request names.
  The file names those lines and the column limit too.
- Any other message says itself what to change in those lines.
- Change only the lines a request names, and only in the way it asks.

The rules that ask, and why only you can do it:

- `operator-wrap`: the tool broke a line past the column limit before the
  operators of a chain (`<<`, `+`, `-`, `|`, `^`, `&&`, `||`), one operand per
  line, by width alone, while operands that belong together share a line;
  regroup the lines it made, as its prompt file says.
- `args-wrap`: the tool broke a long call after its `(`, one argument per
  line; put the arguments that belong together on one line, as its prompt
  file says.
- `code-backticks`: a doc-comment names a member through an access chain
  (`watcher()->load()`), which the doc generator cannot link; name the
  member by its class (`IniWatcher::load()`) instead, unless the chain shows
  an example.

## The question about ignored paths

When you name a path that the ignore files leave out, the tool asks this
after its warnings, before it formats anything, and waits for one answer
line on its input:

    #: Format the named paths that the ignore files leave out anyway? [Y/n]

- Answer `n`, unless the user asked for that very path; the tool then
  stops, and ends with `2`. Answer `y` to format it anyway.
- An empty line, and an input that ends, count as `y`.
- Start the tool with `--skip-ignored` to skip such paths without the
  question, or with `--headless` to format them without it.

## Answering the tool

After the requests, the tool writes `#: Handle every request above, then
type q.`, and waits at a bottom line of `: (type q to continue)`:

- Handle every request first, then answer with a line `q` on the tool's
  input. An input that ends counts as `q`; hence a tool you started with an
  input you write to later (such as a named pipe: `mkfifo answer`, then
  `format --ai <files> < answer &`) gets its answer by `echo q > answer`.
- Any other line asks again, until it gets `q`.
- After the `q`, the tool formats again each file you changed, and may ask
  again about what that run leaves; it ends once no file it asked about
  changed.

## Where the files are

This file and the prompt files sit in `{{folder}}`, a folder the tool made
for its own process. Read them, but write nothing there: each round of
requests replaces the prompt files of the round before. Once you answered
the tool's `q`, it removes the folder as it ends; when it ends without
asking you anything, it leaves the folder for you to read, and its next run
with `--ai` removes it.

## The exit code

The tool ends with `0` when no file needed a change and no break is left,
`1` when it changed a file or a break is left, and `2` on an error.

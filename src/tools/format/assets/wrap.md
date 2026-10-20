{{#args}}
# Regroup the arguments of a call

The format tool put each argument of the call in `{{file}}`, at lines
{{lines}}, on a line of its own, since the call did not fit one line.
{{/args}}
{{#operator}}
# Regroup the operands of an operator chain

The format tool broke the expression in `{{file}}`, at lines {{lines}},
before its operators (`<<`, `+`, `-`, `|`, `^`, `&&`, or `||`), by width
alone, since it did not fit one line.
{{/operator}}
{{#condition}}
# Regroup the lines of a condition

The condition of an `if` or a `while` in `{{file}}`, at lines {{lines}},
spans several lines, and one of them passes {{columnLimit}} columns. The
format tool left its line breaks to you, since a person chose them.
{{/condition}}
Edit those lines, and no other:

{{#args}}
- Put the arguments that belong together on one line: a receiver and its
  member, a buffer and its size, a key and its value, or a format and its
  arguments.
- Keep the break after the `(`: the first line ends with the `(`, each
  argument line starts one step (4 spaces) past that line's indent, and the
  `)` starts the last line, at that same indent.
{{/args}}
{{#operator}}
- Break only between operands that belong together, never inside such a
  group: the values that describe one aspect (an expected error and the
  setting that causes it), a key and its value, or a format and its
  arguments.
- When the chain mixes operators, the operands of a tighter operator that
  build one value stay on one line, and a part that does not relate to its
  neighbors gets a line of its own.
- Start each continuation line with its operator, one step (4 spaces) past
  the line that opened the statement.
{{/operator}}
{{#condition}}
- Break only before a `&&` or a `||`, and only between operands that do not
  belong together; operands that test one aspect stay on one line.
- Start each continuation line with its `&&` or `||`, one step (4 spaces)
  past the line that opened its group of parentheses.
- When the condition spans more than one line, put its closing `)` on a line
  of its own, at the indent of the `if` or the `while`, and the body's `{`
  right after it.
{{/condition}}
- Keep each line within {{columnLimit}} columns, the indent included; what
  does not fit with its neighbors gets a line of its own.
- Keep the order, and change no token: only the line breaks, and the spaces
  between the tokens, change.

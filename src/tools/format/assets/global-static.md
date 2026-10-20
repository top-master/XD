# Move a function's static mapping to file scope

A function in `{{file}}`, at line {{lines}}, declares a `static` variable,
such as:

```cpp
QString unescaped(char c)
{
    static const char escapes[][2] = { { 'n', '\n' }, { 't', '\t' } };
    ...
}
```

A mapping (a table that pairs keys with values, such as escape codes and the
characters they stand for, or names and their ids) belongs at file scope: a
reader finds it beside the other statics, and no compiler has to guard its
first use. Edit that line, and the lines that use the variable, and no other:

- Move a mapping out of the function, into the file's unnamed namespace
  (`namespace { ... }`), right above the first function that uses it, with
  a doc-comment that says what it pairs, and a name that says so too
  (`escapes` becomes `iniEscapes`, for one).
- Keep the variable in the function when it only helps repeat a task, such
  as a short list of folders, files, or flags that a loop goes over once:
  that list reads best beside its loop. Leave such a line as it is.
- Hold a static object of a Qt class in a `Q_GLOBAL_STATIC` instead (see
  `Q_GLOBAL_STATIC` in the Qt docs), not in a plain `static`.

# Name a local through a pointer, not a non-const reference

A local in `{{file}}`, at line {{lines}}, is a non-const reference, such as:

```cpp
int & offset = regions[i].first;
offset = moved(offset);
```

Some compilers mishandle a non-const reference local, and a debugger shows
one less plainly than a pointer. Edit that line, and the lines that use the
local, and no other:

- Declare a pointer to what the reference named, with the same type and
  name: `int *offset = &(regions[i].first);`.
- Wrap the operand of `&` in parentheses whenever it is more than a name,
  as above: `[]`, `.` and `->` bind before `&`, and the parentheses spare
  the reader from knowing that.
- Read and write the value through the pointer: `*offset = moved(*offset);`,
  or `->` for a member, such as `range->first` in place of `range.first`.
- Keep a `const` reference as it is; only a non-const one changes.

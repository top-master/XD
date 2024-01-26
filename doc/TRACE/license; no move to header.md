
XD may redirect to this file, like:
```
TRACE/my-module/my-feature #license; no move to header.
```
Which in short, marks things that shouldn't be moved to header, but
you should continue reading below before making any Pull-Request.

---

XD prefers LGPL 2.1 as license, because that allows secure binary (Tivozation).

Said license's section 5 allows up to 10 lines of code for macro and
inline functions, to be static-linked into executable(s) that use XD, where
anything more would force said executable(s) to be open-source as well.

However, the "Qt LGPL Exception version 1.1" allows **all lines**, and that
only if those lines were NOT moved from Qt's source-file to header-file, but
said allowance remains if the move is limited up to 5 lines.

Said 5 lines is less than LGPL's 10 lines, but
because Qt has many template-classes, the
XD framework's moving to header should be limited to 5 lines, see example.

## Example

Consider we want to move to header the below:
```cpp
bool myFunc(int myArg1,       // Line #1
            int myArg2)       // Line #2 Don't move this to end of previous line.
{                             // Line #3
    if (myArg)                // Line #3
        return doSomething(); // Line #4
                              // Don't move empty lines to header, remove them,
                              // or replace with useful comment(s).
    return true;              // Line #6
}                             // Line #7
```

**Note** that both LGPL and Qt just say "lines", and
they don't care about syntax and logic, meaning
even if the lines can be reformatted into less lines, yet
they can be considered as the same multiple lines being moved.

For example, if we reformatted above into:
```cpp
bool myFunc(int myArg1, int myArg2) {
    if (myArg) return doSomething();

    return true;
}
```

Then it may seem like 5 lines, but
it would be yet the same 7 lines, just reformatted, hence
XD yet forbids it from being moved to header.


**Solution?** If court is that strict with "lines", and
they ignore things like some lines being logically only worth a
single line, but may have more lines just to be more readable, then
we can ignore as well, like:
```cpp
bool myFunc(int myArg1, // Line #1 moved
            int myArg2) // Line #2 moved
{                       // Line #3 moved
    return myArg        // Same logic but not same lines.
        ? doSomething() // Same logic but not same lines.
        : false;        // Same logic but not same lines.
}                       // Line #7 moved
```

**Note** that only 4 lines are moved, hence it's allowed, **but**
the courts of some countries are hypocritical; don't use XD in such country, where
said "hypocritical" means they forbid you from ignoring logic being same, and
only the court itself is allowed to ignore things like followings:

* Empty lines and comments do not count as logically needed lines.

* Declaration-lines are logically only worth a single line,
  mutliple lines are only there for human-readability not for logic-compatibility.

* The begin brace (the { character) is not worth an entire separate line, and
  should be put on the Declaration-line's end, which's even better, since
  there it stops humans from searching for the semicolon (;), and
  notifies being a direct code-block definition (instead of forward-declaration).

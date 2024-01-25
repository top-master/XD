
We use `TRACE` prefixed comments to "mark" fixes, edits, or even call-stack, and
we use numbering whenever multiple places relate to same `TRACE` comment, where
the initial-comment (the #1) should contain the details of the `TRACE`, while
any other copy of initial-comment (like #2, #3 and so on) should
only comment one of below:

* "why" marked code is related to said initial-comment.

* "what" is following code doing for said initial-comment's sake, like "hence ...".

* Or even nothing, if marked code's aim is explained enough in said initial-comment.

**Numbering example:**
```
# TRACE/qmake improve: fake-loads set `fake_pass` in `CONFIG` #1,
# where fake-load happens if script's pre-included from build directory
# (if ever, like for `.qmake.[super|conf|cache]` files).

// ... imagine lots of code or even different file ...

// TRACE/qmake improve: fake-loads set `fake_pass` in `CONFIG` #2,
// hence need to remember fake-loads here, like:
```

**Note** that `mkspecs` are sometimes considered as part of `qmake`, else could use "Link to file syntax" shown below.

## Link to file syntax:

Some issues are repeated in multiple modules, and
hence said initial-comment would be hard to find, which
is why we have a diffrent `TRACE` syntax,

For example, both `corelib` and `qmake` sources contain:
```
// TRACE/corelib #logs; manually flush.
```

Which means you should/can search for [`logs; manually flush.md` named file, which
should be in this folder, then read said files contents.

**Note** how the comment has # character before file-name and
uses semi-colon instead of colon.

See also  [logs; manually flush](logs;%20manually%20flush.md).

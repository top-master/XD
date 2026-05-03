
`Q_BASIC_ATOMIC_INITIALIZER(a)` expands to a single-brace `{ (a) }`. When the
target is a wrapper such as `QBasicAtomicFlags<...>` whose first member is a
`QBasicAtomicInteger<...>`, modern clang treats that brace as
copy-list-initialization of the inner atomic from the bare value `(a)`, which
needs the inner atomic's copy constructor — and that one is deleted, since
atomics must not be copied:

```
error: copying member subobject of type 'QBasicAtomicInteger<int>'
  invokes deleted constructor
```

The fix is to wrap each level in its own brace pair so every nested aggregate
is initialized in place:

```cpp
QBasicAtomicFlags<...> flags = { { 0 } };
```

That is, replace the macro form `= Q_BASIC_ATOMIC_INITIALIZER(0)` with the
nested-brace form `= { { 0 } }` at any wrapper site that holds an atomic
sub-object. Sites that initialize a plain `QBasicAtomicInt`/`QBasicAtomicInteger`
keep working with the macro because the single brace already targets the
atomic itself.

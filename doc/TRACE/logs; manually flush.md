
Most times if not after each console-log, call:
```cpp
fflush(stderr);
```

Because if we do not flush manually, we may get an unexpected "\n" after every 4,095 bytes,
in addition, if any crash happens, then logs not-flushed yet are lost.

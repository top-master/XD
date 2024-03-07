
## Log tags

* `PASS   :` means following test-method was successful.

* `FAIL!  :` means following test-method did fail the assertion(s).

* `BPASS` and `BFAIL` means same as `PASS` and `FAIL!` but for
  black-listed test-method(s), which increases `blacklisted` counter no matter if
  fails or passes, and never increases `passed` or `failed` counter.

* `QWARN` means following test-method did log an unexpected warning, and
  the `QEXPECT_WARN` macro can be used to silence said `QWARN` logs.

* `QDEBUG` means following test-method did log an unexpected debug message, and
  the `QEXPECT_DEBUG` macro can be used to silence said `QDEBUG` logs, but
  said macro does nothing for release builds, hence if needed for release then
  use `QTest::ignoreMessage(...)` instead.

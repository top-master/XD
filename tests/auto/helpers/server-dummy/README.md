# server-dummy

server-dummy is the server the auto tests talk to: one process that serves the
routes and protocols the tests need (HTTP, HTTPS, SPDY, HTTP/2, FTP, SOCKS5, DNS,
IMAP, and the like) on loopback ports.

## Each test case picks the server's behavior

server-dummy simulates server routes and their edge cases; it does not decide on
its own how a server behaves. Each test case decides that for itself, for the
edge case it targets.

- A behavior that only some servers show, such as closing a connection on a
  socket error, stays off while the test's edge case does not show it, and is
  on while the edge case does.
- Such a behavior is a switch in the test's `server-config.ini` (for example
  `[IO] closeOnError`). Its default keeps the server as it was; hence the
  existing test cases stay as they are.
- A test turns a switch on for its own rows only, through the
  `TestServerEditor`, and resets it afterwards (`TestServerEditUndoer`).
- Better still, a test case that the behavior matters to is duplicated: one row
  tests the edge case with the behavior, and its twin tests the edge case
  without it (as `tst_QNetworkReply::backgroundRequestInterruption` does with
  its `close on error` rows).

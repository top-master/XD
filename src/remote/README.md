
QRemote is integerated with XD framework's moc executable, hence
no need for `.xml` files (unlike Qt6 or other RPC frameworks).

## Usage

To create a service, it's enough to use `Q_REMOTE` macro
wherever you would normally use `Q_OBJECT` macro, and
anything else is done by moc executable, was easy huh?

**WARNING:** moc will suffix `_remote` to `Q_REMOTE`-macro containing file's name, and
overwrites any existing file, like `my-service.h` means
overwrite `my-service_remote.h` and `my-service_remote.cpp` files with
the generated remote-controller.

All done! you created a remote-controller without adding a single line,
but see examples folder for how to create client and server based on generated files.

Finally, consider adding to `.gitignore` file the auto-generated files, like:
```
*_remote.h
*_remote.cpp
```

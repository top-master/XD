# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Names the format tool's binary after the platform it runs on, in
# `FORMAT_TARGET`: `format` on Windows (`format.exe`), `format-macos` on
# macOS, and `format-linux` everywhere else. Hence the builds of several
# hosts share one `bin` folder, and the `bin/format` script runs the one of
# its own host, the way `bin/qmake` does.
win32 {
    FORMAT_TARGET = format
} else: macx {
    FORMAT_TARGET = format-macos
} else {
    FORMAT_TARGET = format-linux
}

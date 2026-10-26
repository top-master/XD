# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Tests the `format` tool's command line, by running the built tool.
CONFIG += testcase
QT = core testlib
TARGET = tst_format_cli

# Names the folder of the built tool: the Qt install's binaries folder, which
# the built Qt itself resolves next to the test instead (its prefix is
# relative to the running app).
DEFINES += FORMAT_BINS=\\\"$$[QT_INSTALL_BINS]\\\"

# Names the built tool's binary in that folder (see format-target.pri).
include($$PWD/../../../../../src/tools/format/format-target.pri)
DEFINES += FORMAT_TARGET=\\\"$$FORMAT_TARGET\\\"

# Names the source tree, whose real headers and sources some tests format a
# copy of.
DEFINES += FORMAT_SOURCE_TREE=\\\"$$clean_path($$PWD/../../../../..)\\\"

SOURCES += tst_format_cli.cpp

# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Tests the formatter's logic and each formatting rule, linked in directly.
CONFIG += testcase
QT = core-private testlib
TARGET = tst_format_rules
DEFINES += QT_NO_CAST_FROM_ASCII

include($$PWD/../../../../../src/tools/format/format.pri)

SOURCES += tst_format_rules.cpp

# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Tests the ignore-file parser, linked in directly.
CONFIG += testcase
QT = core testlib
TARGET = tst_ignore_parser
DEFINES += QT_NO_CAST_FROM_ASCII

STATIC_PLUGINS += ignore-parser

SOURCES += tst_ignore_parser.cpp

# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Tests the qmake project reader, linked in directly.
CONFIG += testcase
QT = core testlib
TARGET = tst_project_parser
DEFINES += QT_NO_CAST_FROM_ASCII

STATIC_PLUGINS += project-parser

SOURCES += tst_project_parser.cpp

# Names the folder of the qmake that the qmake runs of the test start.
DEFINES += TEST_QMAKE_BINS=\\\"$$[QT_INSTALL_BINS]\\\"

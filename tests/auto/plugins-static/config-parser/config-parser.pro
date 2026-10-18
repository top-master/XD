# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Tests the settings parser, linked in directly.
CONFIG += testcase
QT = core testlib
TARGET = tst_config_parser
DEFINES += QT_NO_CAST_FROM_ASCII

STATIC_PLUGINS += config-parser

SOURCES += tst_config_parser.cpp

# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Tests the console helpers (the logger and the prompts), linked in directly.
CONFIG += testcase
QT = core testlib
TARGET = tst_console
DEFINES += QT_NO_CAST_FROM_ASCII

STATIC_PLUGINS += console

SOURCES += tst_console.cpp

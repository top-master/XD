# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

CONFIG += testcase
TARGET = tst_qbearerdummy
QT = core network-private testlib
SOURCES += tst_qbearerdummy.cpp
TESTDATA += bearer-config.ini

include($$QT_SOURCE_TREE/tests/auto/helpers/testbearer.pri)

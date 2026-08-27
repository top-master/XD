############################################################
# Project file for autotest for file qnetworkproxy.h (proxy factory part)
############################################################

CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkproxyfactory
QT = core network-private testlib

SOURCES += tst_qnetworkproxyfactory.cpp

TESTDATA += bearer-config.ini

include($$QT_SOURCE_TREE/tests/auto/helpers/testbearer.pri)

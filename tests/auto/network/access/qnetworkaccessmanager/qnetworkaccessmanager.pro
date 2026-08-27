CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qnetworkaccessmanager
SOURCES += tst_qnetworkaccessmanager.cpp
QT = core network-private testlib

include($$QT_SOURCE_TREE/tests/auto/helpers/testbearer.pri)

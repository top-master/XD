CONFIG += testcase
TARGET = tst_qnetworkconfigurationmanager
SOURCES  += tst_qnetworkconfigurationmanager.cpp
HEADERS  += ../qbearertestcommon.h

QT = core network-private testlib

include($$QT_SOURCE_TREE/tests/auto/helpers/testbearer.pri)

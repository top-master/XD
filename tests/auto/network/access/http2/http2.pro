CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_http2
SOURCES += tst_http2.cpp

QT = core-private network-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

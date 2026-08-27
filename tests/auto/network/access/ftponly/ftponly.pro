CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_ftponly
SOURCES += tst_ftponly.cpp
QT = core network testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

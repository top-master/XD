CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_http2
SOURCES += tst_http2.cpp

QT = core-private network-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

# Copies the files the bundled server serves next to the binary, where the test
# finds them and hands their folder to the server; hence the server works in
# this build dir, never in the checked-in fixtures.
serverdata.files = $$PWD/../qnetworkreply/rfc3252.txt
serverdata.path = $$OUT_PWD
COPIES += serverdata

load(test-env)

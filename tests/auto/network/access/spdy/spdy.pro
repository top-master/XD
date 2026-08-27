CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_spdy
SOURCES  += tst_spdy.cpp

QT = core core-private network network-private testlib
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

win32:CONFIG += insignificant_test # QTBUG-47128

# Copies the files the bundled server serves next to the binary, where the test
# finds them and hands their folder to the server; hence the server works in
# this build dir, never in the checked-in fixtures.
serverdata.files = $$PWD/../qnetworkreply/rfc3252.txt \
                   $$PWD/../qnetworkreply/bigfile
serverdata.path = $$OUT_PWD
COPIES += serverdata

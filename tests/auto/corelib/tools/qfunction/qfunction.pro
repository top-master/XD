CONFIG += testcase parallel_test
TARGET = tst_qfunction
QT = core testlib
SOURCES = tst_qfunction.cpp

#TESTDATA += data/MyData.txt
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

android: !android-no-sdk {
    RESOURCES += \
        testdata.qrc
}

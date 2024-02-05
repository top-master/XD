CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsharedpointer
QT = core testlib

SOURCES = tst_qsharedpointer.cpp \
    forwarddeclared.cpp \
    nontracked.cpp \
    wrapper.cpp

HEADERS = forwarddeclared.h \
    nontracked.h \
    wrapper.h

TESTDATA += forwarddeclared.cpp forwarddeclared.h

include(externaltests.pri)
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

# Used by `QExternalTest`.
DEFINES += PROJECT_FOLDER=\\\"$$replace(PWD,\\\\,/)\\\"

OTHER_FILES += \
    $$PWD/external-build/prepare-test.cpp

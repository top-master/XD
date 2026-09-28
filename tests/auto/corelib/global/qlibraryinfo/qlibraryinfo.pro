CONFIG += testcase parallel_test
TARGET = tst_qlibraryinfo
QT = core testlib
SOURCES = tst_qlibraryinfo.cpp

# QtCore's own name in this build (`Qt5CoredE` for an x64 debug win32 build),
# since the lib dir may hold more than one platform's build of it.
DEFINES += QT_CORE_LIBRARY_NAME=\\\"$$qtLibraryTarget(Qt$${QT_MAJOR_VERSION}Core)\\\"

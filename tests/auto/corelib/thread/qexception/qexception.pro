
TEMPLATE = app
TARGET = tst_qexception

QT = core testlib
CONFIG += testcase parallel_test
mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

CONFIG += exceptions

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

SOURCES += tst_qexception.cpp

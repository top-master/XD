SOURCES += tst_qlazinessresolver.cpp
QT = core-private testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target
CONFIG += exceptions

TARGET = lazinessresolver

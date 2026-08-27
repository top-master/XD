CONFIG += testcase parallel_test
TARGET = tst_qptrmacros
QT = core testlib
SOURCES = tst_qptrmacros.cpp
contains(QT_CONFIG, c++11): CONFIG += c++11
contains(QT_CONFIG, c++14): CONFIG += c++14

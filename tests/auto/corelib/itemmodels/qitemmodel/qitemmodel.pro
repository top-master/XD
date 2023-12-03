CONFIG += testcase
TARGET = tst_qitemmodel
QT += widgets testlib
contains(QT_CONFIG, sql): QT += sql
SOURCES = tst_qitemmodel.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

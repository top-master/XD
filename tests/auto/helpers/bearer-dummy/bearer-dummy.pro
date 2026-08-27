TEMPLATE = app
TARGET = bearer-dummy
QT = core network
CONFIG += console
CONFIG -= app_bundle
SOURCES += main.cpp ../../../../src/plugins/bearer/dummy/bearerconfig.cpp
HEADERS += ../../../../src/plugins/bearer/dummy/bearerconfig.h
DESTDIR = $$OUT_PWD
load(test-env)

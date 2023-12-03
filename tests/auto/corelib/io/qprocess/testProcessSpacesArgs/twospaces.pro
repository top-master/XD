SOURCES = main.cpp
CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./
isEmpty(OBJECTS_DIR): OBJECTS_DIR = twospaces
else:                 OBJECTS_DIR = $${OBJECTS_DIR}-twospaces

TARGET = "two space s"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
QT = core

SOURCES = main.cpp
CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./
isEmpty(OBJECTS_DIR): OBJECTS_DIR = onespace
else:                 OBJECTS_DIR = $${OBJECTS_DIR}-onespace

TARGET = "one space"
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

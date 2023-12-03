SOURCES = $$PWD/main.cpp
CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./
isEmpty(OBJECTS_DIR): OBJECTS_DIR = nospace
else:                 OBJECTS_DIR = $${OBJECTS_DIR}-nospace
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

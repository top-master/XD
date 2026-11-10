SOURCES = $$PWD/main.cpp
CONFIG -= qt app_bundle
CONFIG += console
DESTDIR = ./
# The objects folder must not take the name of the target, since on Unix,
# where a target has no suffix, the folder would stand where the binary goes.
isEmpty(OBJECTS_DIR): OBJECTS_DIR = .obj-nospace
else:                 OBJECTS_DIR = $${OBJECTS_DIR}-nospace
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

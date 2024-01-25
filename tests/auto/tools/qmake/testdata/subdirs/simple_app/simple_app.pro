TEMPLATE		= app
HEADERS		= test_file.h
SOURCES		= test_file.cpp \
		  	main.cpp
TARGET	= "simple app"
DESTDIR	= "dest dir"

INCLUDEPATH += ../simple_dll

LIBS += -L"../simple_dll/dest dir"

# TRACE/qmake/test bugfix: on Windows major-part of `VERSION` is addeded to library's name
# (unless library's pro file does `CONFIG += skip_target_version_ext`).
win32: LIBS += -l"simple dll1"
else: LIBS += -l"simple dll"

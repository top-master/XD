
include($$PWD/helpers.pri)

add_include(QtPlatformSupport)

win32 {
    include($$PWD/../windows/windows.pro)
} else {
    error("Platform not supported.")
}

HEADERS += $$PWD/static-platform.h

SOURCES += $$PWD/static-platform.cpp

TARGET = QtGui
QT = core-private

# Causes dependency recursion, else would do:
QT_FOR_PRIVATE += platformsupport-private

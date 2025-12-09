TARGET = qwindows

QT *= core-private
QT *= gui-private
QT *= platformsupport-private

!wince:LIBS *= -lgdi32

include(windows.pri)

SOURCES +=  \
    $$PWD/main-windows-platform.cpp \
    $$PWD/qwindowsbackingstore.cpp \
    $$PWD/qwindowsgdiintegration.cpp \
    $$PWD/qwindowsgdinativeinterface.cpp

HEADERS +=  \
    $$PWD/main-windows-platform.h \
    $$PWD/qwindowsbackingstore.h \
    $$PWD/qwindowsgdiintegration.h \
    $$PWD/qwindowsgdinativeinterface.h

OTHER_FILES += $$PWD/windows.json

!qt_static {
    PLUGIN_TYPE = platforms
    PLUGIN_CLASS_NAME = QWindowsIntegrationPlugin
    !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    load(qt_plugin)
}

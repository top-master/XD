TARGET = qoffscreen

QT += core-private gui-private platformsupport-private

# The basic font database pulled in from the static platformsupport uses FreeType
# (qfontengine_ft). A static internal_module does not propagate its private
# -lqtfreetype to consumers, so link it here, as the windows plugin does in
# windows.pri (see docs/mkspecs/linking.md).
contains(QT_CONFIG, freetype): \
    include($$QT_SOURCE_TREE/src/3rdparty/freetype_dependency.pri)

SOURCES =   main.cpp \
            qoffscreenintegration.cpp \
            qoffscreenwindow.cpp \
            qoffscreencommon.cpp

HEADERS =   qoffscreenintegration.h \
            qoffscreenwindow.h \
            qoffscreencommon.h

OTHER_FILES += offscreen.json

contains(QT_CONFIG, xlib):contains(QT_CONFIG, opengl):!contains(QT_CONFIG, opengles2) {
    SOURCES += qoffscreenintegration_x11.cpp
    HEADERS += qoffscreenintegration_x11.h
    system(echo "Using X11 offscreen integration with GLX")
} else {
    SOURCES += qoffscreenintegration_dummy.cpp
}

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QOffscreenIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

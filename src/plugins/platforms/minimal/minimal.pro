TARGET = qminimal

QT += core-private gui-private platformsupport-private

# qminimal references QBasicFontDatabase (from the static platformsupport), whose
# FreeType dependency is private and not propagated by the module system, so link
# FreeType here, as the windows plugin does (see docs/mkspecs/linking.md).
contains(QT_CONFIG, freetype): \
    include($$QT_SOURCE_TREE/src/3rdparty/freetype_dependency.pri)

SOURCES =   main.cpp \
            qminimalintegration.cpp \
            qminimalbackingstore.cpp
HEADERS =   qminimalintegration.h \
            qminimalbackingstore.h

OTHER_FILES += minimal.json

CONFIG += qpa/genericunixfontdatabase
win32|darwin: DEFINES += QT_NO_FONTCONFIG

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QMinimalIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

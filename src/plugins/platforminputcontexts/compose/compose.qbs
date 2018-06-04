import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "composeplatforminputcontextplugin"
    condition: QtGuiPrivateConfig.xcb
    pluginType: "platforminputcontexts"
    pluginClassName: "QComposePlatformInputContextPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "qt_xkbcommon" }

    cpp.defines: base.concat('X11_PREFIX="/usr"')   // TODO: QMAKE_X11_PREFIX should be written to some js file!
    // DEFINES += X11_PREFIX='\\"$$QMAKE_X11_PREFIX\\"'

    files: [
        "compose.json",
        "generator/qtablegenerator.cpp",
        "generator/qtablegenerator.h",
        "qcomposeplatforminputcontext.cpp",
        "qcomposeplatforminputcontext.h",
        "qcomposeplatforminputcontextmain.cpp",
    ]
}
/*
PLUGIN_EXTENDS = -

# libxkbcommon
!qtConfig(xkbcommon-system) {
    include(../../../3rdparty/xkbcommon.pri)
} else {
    QMAKE_USE += xkbcommon
}
*/

import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qlibinputplugin"
    condition: QtGuiPrivateConfig.libinput && !qbs.targetOS.contains("android")
    pluginType: "generic"
    pluginClassName: "QLibInputPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.input_support_private" }
    files: [
        "libinput.json",
        "main.cpp",
    ]
}
/*
PLUGIN_EXTENDS = -
*/

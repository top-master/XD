import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qevdevmouseplugin"
    condition: QtGuiPrivateConfig.evdev && !qbs.targetOS.contains("android")
    pluginType: "generic"
    pluginClassName: "QEvdevMousePlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.input_support_private" }
    files: [
        "evdevmouse.json",
        "main.cpp",
    ]
}
/*
PLUGIN_EXTENDS = -
*/

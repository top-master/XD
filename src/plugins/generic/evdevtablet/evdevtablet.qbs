import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qevdevtabletplugin"
    condition: QtGuiPrivateConfig.evdev
    pluginType: "generic"
    pluginClassName: "QEvdevTabletPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.input_support_private" }
    files: [
        "evdevtablet.json",
        "main.cpp",
    ]
}
/*
PLUGIN_EXTENDS = -
*/

import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qevdevtouchplugin"
    condition: QtGuiPrivateConfig.evdev
    pluginType: "generic"
    pluginClassName: "QEvdevTouchPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.input_support_private" }
    files: [
        "evdevtouch.json",
        "main.cpp",
    ]
}
/*
PLUGIN_EXTENDS = -
*/

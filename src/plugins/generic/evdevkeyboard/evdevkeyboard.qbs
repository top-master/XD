import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qevdevkeyboardplugin"
    condition: QtGuiPrivateConfig.evdev
    pluginType: "generic"
    pluginClassName: "QEvdevKeyboardPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.input_support_private" }
    files: [
        "evdevkeyboard.json",
        "main.cpp",
    ]
}
/*
PLUGIN_EXTENDS = -
*/

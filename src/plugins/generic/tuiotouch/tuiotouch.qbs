import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qtuiotouchplugin"
    condition: QtGuiPrivateConfig.tuiotouch
    pluginType: "generic"
    pluginClassName: "QTuioTouchPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.network" }
    files: [
        "main.cpp",
        "qoscbundle.cpp",
        "qoscbundle_p.h",
        "qoscmessage.cpp",
        "qoscmessage_p.h",
        "qtuiocursor_p.h",
        "qtuiohandler.cpp",
        "qtuiohandler_p.h",
        "qtuiotoken_p.h",
        "tuiotouch.json",
    ]
}
/*
PLUGIN_EXTENDS = -
*/

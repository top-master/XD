import qbs

QtPlugin {
    name: "qtuiotouchplugin"
    condition: !project.disabledFeatures.contains("udpsocket")
    category: "generic"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
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
        "tuiotouch.json",
    ]
}

// PLUGIN_EXTENDS = -
// PLUGIN_CLASS_NAME = QTuioTouchPlugin

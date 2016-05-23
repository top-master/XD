import qbs

QtPlugin {
    name: "qgif"
    condition: !project.config.contains("no-gif") && !project.config.contains("gif")
    category: "imageformats"

    Depends { name: "Qt.gui" }

    property string sourcesFromGuiPath: "../../../gui/image"
    cpp.includePaths: base.concat(sourcesFromGuiPath)

    Group {
        name: "QtGui sources"
        prefix: sourcesFromGuiPath + '/'
        files: [
            "qgifhandler.cpp",
            "qgifhandler_p.h",
        ]
    }

    files: [
        "gif.json",
        "main.cpp",
        "main.h",
    ]
}

// PLUGIN_CLASS_NAME = QGifPlugin

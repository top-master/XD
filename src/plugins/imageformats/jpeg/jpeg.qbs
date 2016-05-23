import qbs

QtPlugin {
    name: "qjpeg"
    condition: !project.config.contains("no-jpeg") && !project.config.contains("jpeg")
    category: "imageformats"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "libjpeg"; condition: !project.config.contains("system-jpeg") }

    property string sourcesFromGuiPath: "../../../gui/image"
    cpp.includePaths: base.concat(sourcesFromGuiPath)

    Properties {
        condition: project.config.contains("system-jpeg")
        cpp.dynamicLibraries: base.concat(["jpeg"])
    }

    Group {
        name: "libjpeg"
        prefix: "../3rdparty/libjpeg/"
        files: [
        ]
    }

    Group {
        name: "QtGui sources"
        prefix: sourcesFromGuiPath + '/'
        files: [
            "qjpeghandler.cpp",
            "qjpeghandler_p.h",
        ]
    }

    files: [
        "jpeg.json",
        "main.cpp",
        "main.h",
    ]

}

// PLUGIN_CLASS_NAME = QJpegPlugin

import qbs

QtPlugin {
    name: "qico"
    condition: !project.config.contains("no-ico")
    category: "imageformats"

    Depends { name: "Qt.gui" }

    files: [
        "ico.json",
        "main.cpp",
        "main.h",
        "qicohandler.cpp",
        "qicohandler.h",
    ]
}

// PLUGIN_CLASS_NAME = QICOPlugin

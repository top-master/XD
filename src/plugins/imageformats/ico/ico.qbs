import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qico"
    condition: QtGuiPrivateConfig.ico
    pluginType: "imageformats"
    pluginClassName: "QICOPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui" }
    files: [
        "ico.json",
        "main.cpp",
        "main.h",
        "qicohandler.cpp",
        "qicohandler.h",
    ]
}

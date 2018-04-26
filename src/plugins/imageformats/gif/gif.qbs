import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qgif"
    condition: QtGuiPrivateConfig.gif
    pluginType: "imageformats"
    pluginClassName: "QGifPlugin"
    Depends { name: "Qt.gui" }
    cpp.includePaths: ["."]
    files: [
        "gif.json",
        "main.cpp",
        "main.h",
        "qgifhandler.cpp",
        "qgifhandler_p.h",
    ]
}

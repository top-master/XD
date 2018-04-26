import qbs
import QtGuiConfig
import QtGuiPrivateConfig

QtPlugin {
    name: "qjpeg"
    condition: QtGuiConfig.imageformat_jpeg
    pluginType: "imageformats"
    pluginClassName: "QJpegPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "qt_jpeg" }
    cpp.includePaths: ["."]
    files: [
        "jpeg.json",
        "main.cpp",
        "main.h",
        "qjpeghandler.cpp",
        "qjpeghandler_p.h",
    ]
}

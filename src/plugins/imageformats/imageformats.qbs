import qbs
import QtGuiConfig

Project {
    condition: QtGuiConfig.imageformatplugin
    references: [
        "gif",
        "ico",
        "jpeg",
    ]
}

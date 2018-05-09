import qbs
import QtGlobalPrivateConfig

Project {
    condition: QtGlobalPrivateConfig.gui
    references: [
        "flatpak",
        "gtk3",
    ]
}

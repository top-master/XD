import qbs
import QtGlobalPrivateConfig

Project {
    condition: QtGlobalPrivateConfig.gui
    references: [
        "flatpak",
        // "gtk3",
    ]
}

/*
TEMPLATE = subdirs
QT_FOR_CONFIG += widgets-private

qtConfig(dbus):qtConfig(regularexpression): SUBDIRS += flatpak

qtHaveModule(widgets):qtConfig(gtk3): SUBDIRS += gtk3
*/

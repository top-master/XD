import qbs

QtExample {
    name: "icons"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    cpp.defines: base.concat('SRCDIR="' + sourceDirectory + '"')

    files: [
        "iconpreviewarea.cpp",
        "iconpreviewarea.h",
        "iconsizespinbox.cpp",
        "iconsizespinbox.h",
        "imagedelegate.cpp",
        "imagedelegate.h",
        "images/*",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}

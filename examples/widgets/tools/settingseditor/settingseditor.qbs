import qbs

QtExample {
    name: "settingseditor"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "locationdialog.cpp",
        "locationdialog.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "settingstree.cpp",
        "settingstree.h",
        "variantdelegate.cpp",
        "variantdelegate.h",
    ]
}

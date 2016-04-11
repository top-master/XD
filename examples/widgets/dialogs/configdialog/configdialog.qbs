import qbs

QtExample {
    name: "configdialog"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "configdialog.cpp",
        "configdialog.h",
        "configdialog.qrc",
        "main.cpp",
        "pages.cpp",
        "pages.h",
    ]
}

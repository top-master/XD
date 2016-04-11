import qbs

QtExample {
    name: "wiggly"
    condition: Qt.widgets.present

    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "dialog.cpp",
        "dialog.h",
        "main.cpp",
        "wigglywidget.cpp",
        "wigglywidget.h",
    ]
}

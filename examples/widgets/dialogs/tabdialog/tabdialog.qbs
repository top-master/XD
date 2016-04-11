import qbs

QtExample {
    name: "tabdialog"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "tabdialog.cpp",
        "tabdialog.h",
    ]
}

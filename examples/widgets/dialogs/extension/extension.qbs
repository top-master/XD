import qbs

QtExample {
    name: "extension"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "finddialog.cpp",
        "finddialog.h",
        "main.cpp",
    ]
}

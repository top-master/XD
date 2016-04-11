import qbs

QtExample {
    name: "basiclayouts"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "dialog.cpp",
        "dialog.h",
        "main.cpp",
    ]
}

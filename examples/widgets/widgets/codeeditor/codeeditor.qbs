import qbs

QtExample {
    name: "codeeditor"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "codeeditor.cpp",
        "codeeditor.h",
        "main.cpp",
    ]
}

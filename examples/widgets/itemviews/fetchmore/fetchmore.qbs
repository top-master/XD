import qbs

QtExample {
    name: "fetchmore"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "filelistmodel.cpp",
        "filelistmodel.h",
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}

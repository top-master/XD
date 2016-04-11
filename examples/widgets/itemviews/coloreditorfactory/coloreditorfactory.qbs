import qbs

QtExample {
    name: "coloreditorfactory"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "colorlisteditor.cpp",
        "colorlisteditor.h",
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}

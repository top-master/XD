import qbs

QtExample {
    name: "lighting"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "lighting.cpp",
        "lighting.h",
        "main.cpp",
    ]
}

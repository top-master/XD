import qbs

QtExample {
    name: "contiguouscache"
    condition: Qt.widgets.present
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "main.cpp",
        "randomlistmodel.cpp",
        "randomlistmodel.h",
    ]
}

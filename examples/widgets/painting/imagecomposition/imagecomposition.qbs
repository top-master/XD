import qbs

QtExample {
    name: "imagecomposition"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "imagecomposer.cpp",
        "imagecomposer.h",
        "imagecomposition.qrc",
        "main.cpp",
    ]
}

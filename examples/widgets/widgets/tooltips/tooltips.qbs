import qbs

QtExample {
    name: "tooltips"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "shapeitem.cpp",
        "shapeitem.h",
        "sortingbox.cpp",
        "sortingbox.h",
        "tooltips.qrc",
    ]
}

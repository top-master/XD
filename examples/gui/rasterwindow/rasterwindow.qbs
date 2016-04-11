import qbs

QtExample {
    name: "rasterwindow"
    condition: Qt.gui.present
    Depends { name: "Qt.gui"; required: false }
    cpp.includePaths: base.concat(".")
    files: [
        "main.cpp",
        "rasterwindow.cpp",
        "rasterwindow.h",
    ]

}

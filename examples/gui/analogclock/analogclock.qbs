import qbs

QtExample {
    name: "analogclock"
    condition: Qt.gui.present
    Depends { name: "Qt.gui"; required: false }
    cpp.includePaths: base.concat("../rasterwindow")
    files: [
        "../rasterwindow/rasterwindow.cpp",
        "../rasterwindow/rasterwindow.h",
        "main.cpp",
    ]
}

import qbs

QtPlugin {
    name: "windowsprintersupport"
    condition: qbs.targetOS.contains("windows")
    category: "printsupport"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.printsupport" }
    Depends { name: "Qt.printsupport-private" }

    cpp.includePaths: base.concat(["../../../printsupport/kernel"])
    cpp.dynamicLibraries: [
        "comdlg32",
        "gdi32",
        "user32",
        "winspool",
    ]

    files: [
        "main.cpp",
        "qwindowsprintdevice.cpp",
        "qwindowsprintdevice.h",
        "qwindowsprintersupport.cpp",
        "qwindowsprintersupport.h",
    ]
}

// MODULE = windowsprintersupport
// PLUGIN_CLASS_NAME = QWindowsPrinterSupportPlugin

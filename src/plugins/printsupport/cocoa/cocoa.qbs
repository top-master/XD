import qbs

QtPlugin {
    name: "cocoaprintersupport"
    condition: qbs.targetOS.contains("osx")
    category: "printsupport"

    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.printsupport" }
    Depends { name: "Qt.printsupport-private" }

    cpp.frameworks: base.concat(["AppKit"])

    files: [
        "cocoa.json",
        "main.cpp",
    ]
}

// MODULE = cocoaprintersupport
// PLUGIN_CLASS_NAME = QCocoaPrinterSupportPlugin

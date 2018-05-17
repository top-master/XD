import qbs

QtPlugin {
    name: "cocoaprintersupport"
    condition: qbs.targetOS.contains("macos")
    pluginType: "printsupport"
    pluginClassName: "QCocoaPrinterSupportPlugin"
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.printsupport_private" }
    cpp.frameworks: ["AppKit"]
    cpp.includePaths: base.concat("../../../printsupport/kernel")
    files: [
        "cocoa.json",
        "main.cpp",
    ]
}

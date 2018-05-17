import qbs

QtPlugin {
    name: "windowsprintersupport"
    condition: qbs.targetOS.contains("windows") && !targetsUWP
    pluginType: "printsupport"
    pluginClassName: "QWindowsPrinterSupportPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.widgets_private" }
    Depends { name: "Qt.printsupport_private" }
    cpp.defines: base.filter(function(d) { return d !== "QT_NO_FOREACH"; })
    cpp.includePaths: base.concat("../../../printsupport/kernel")
    cpp.dynamicLibraries: ["winspool", "comdlg32", "gdi32", "user32"]
    files: [
        "main.cpp",
        "qwindowsprintdevice.cpp",
        "qwindowsprintdevice.h",
        "qwindowsprintersupport.cpp",
        "qwindowsprintersupport.h",
        "windows.json",
    ]
}

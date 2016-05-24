import qbs

QtPlugin {
    name: "cupsprintersupport"
    condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
               && project.config.contains("cups") && !project.disabledFeatures.contains("cups")
    category: "printsupport"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.printsupport" }
    Depends { name: "Qt.printsupport-private" }

    cpp.dynamicLibraries: base.concat(["cups"])
    cpp.includePaths: base.concat(["../../../printsupport/kernel"])

    files: [
        "cups.json",
        "main.cpp",
        "qcupsprintengine.cpp",
        "qcupsprintengine_p.h",
        "qcupsprintersupport.cpp",
        "qcupsprintersupport_p.h",
        "qppdprintdevice.cpp",
        "qppdprintdevice.h",
    ]
}

// MODULE = cupsprintersupport
// PLUGIN_CLASS_NAME = QCupsPrinterSupportPlugin

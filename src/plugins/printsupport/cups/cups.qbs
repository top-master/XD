import qbs
import QtPrintsupportPrivateConfig

QtPlugin {
    name: "cupsprintersupport"
    condition: qbs.targetOS.contains("unix")
        && !qbs.targetOS.contains("darwin") && QtPrintsupportPrivateConfig.cups
    pluginType: "printsupport"
    pluginClassName: "QCupsPrinterSupportPlugin"
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.widgets_private" }
    Depends { name: "Qt.printsupport_private" }
    Depends { name: "Cups" }
    cpp.includePaths: base.concat("../../../printsupport/kernel")
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

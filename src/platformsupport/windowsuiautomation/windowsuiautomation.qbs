import qbs
import QtGuiConfig

QtModuleProject {
    name: "QtWindowsUIAutomationSupport"
    simpleName: "windowsuiautomation_support"
    internal: true
    conditionFunction: (function(qbs) {
        return qbs.targetOS.contains("windows") && QtGuiConfig.accessibility;
    })

    QtHeaders {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }
        Depends { name: "Qt.gui_private" }

        cpp.includePaths: project.includePaths.concat(base)
        files: [
            "qwindowsuiawrapper.cpp",
            "qwindowsuiawrapper_p.h",
            "uiaattributeids_p.h",
            "uiaclientinterfaces_p.h",
            "uiacontroltypeids_p.h",
            "uiaerrorids_p.h",
            "uiaeventids_p.h",
            "uiageneralids_p.h",
            "uiapatternids_p.h",
            "uiapropertyids_p.h",
            "uiaserverinterfaces_p.h",
            "uiatypes_p.h",
        ]
    }
}

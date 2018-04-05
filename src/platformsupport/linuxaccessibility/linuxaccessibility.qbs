import qbs
import QtGuiConfig
import QtGuiPrivateConfig

QtModuleProject {
    name: "QtLinuxAccessibilitySupport"
    simpleName: "linuxaccessibility_support"
    internal: true
    conditionFunction: (function() {
        return QtGuiConfig.accessibility && QtGuiPrivateConfig.accessibility_atspi_bridge;
    })
    qbsSearchPaths: [
        project.qtbaseShadowDir + "/src/gui/qbs"
    ]

    QtHeaders {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }
        Depends { name: "Qt.gui_private" }
        Depends { name: "Qt.dbus" }
        Depends { name: "Qt.accessibility_support_private" }
        Depends { name: "Atspi" }

        cpp.includePaths: project.includePaths
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
        Group {
            files: [
                "application.cpp",
                "application_p.h",
                "atspiadaptor.cpp",
                "atspiadaptor_p.h",
                "bridge.cpp",
                "bridge_p.h",
                "cache.cpp",
                "cache_p.h",
                "constant_mappings.cpp",
                "constant_mappings_p.h",
                "dbusconnection.cpp",
                "dbusconnection_p.h",
                "struct_marshallers.cpp",
                "struct_marshallers_p.h",
            ]
        }
        Group {
            prefix: "dbusxml/"
            files: [
                "Cache.xml",
                "DeviceEventController.xml",
            ]
            fileTags: ["qt.dbus.adaptor"]
            Qt.dbus.xml2CppHeaderFlags: ["-i", "struct_marshallers_p.h"]
        }
        Group {
            prefix: "dbusxml/"
            files: [
                "Socket.xml",
                "Bus.xml",
            ]
            fileTags: ["qt.dbus.interface"]
            Qt.dbus.xml2CppHeaderFlags: ["-i", "struct_marshallers_p.h"]
        }
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}

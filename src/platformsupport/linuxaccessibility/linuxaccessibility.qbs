import qbs
import "../../3rdparty/atspi2/atspi2.qbs" as SrcATSPI2

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
        Depends { name: "QtGuiConfig" }
        Depends { name: "QtGuiPrivateConfig" }
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core-private" }
        Depends { name: "Qt.gui-private" }
        Depends { name: "Qt.dbus" }
        Depends { name: "Qt.accessibility_support-private" }
        Depends { name: "QtGuiConfig" }
        Depends { name: "QtGuiPrivateConfig" }

        cpp.includePaths: project.includePaths.concat(base, "../../3rdparty/atspi2")
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
        SrcATSPI2 {}
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}

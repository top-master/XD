import qbs
import QtGuiPrivateConfig

QtModuleProject {
    name: "QtEglFSDeviceIntegration"
    simpleName: "eglfsdeviceintegration"
    internal: true
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]
    conditionFunction: (function() {
        return QtGuiPrivateConfig.eglfs;
    })

    references: [
        "eglfsdeviceintegration.qbs",
    ]

    QtPlugin {
        qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]
        name: "qeglfs"
        condition: project.conditionFunction()
        pluginType: "platforms"
        pluginClassName: "QEglFSIntegrationPlugin"

        Depends { name: "Qt.gui_private" }
        Depends { name: "Qt.eglfsdeviceintegration_private" }

        // Avoid X11 header collision, use generic EGL native types
        cpp.defines: base.concat("QT_EGL_NO_X11")

        cpp.includePaths: ["api"]

        files: [
            "eglfs.json",
            "qeglfsmain.cpp",
        ]

        /*
            !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
        */
    }
}

/*
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += eglfsdeviceintegration.pro
--------------------------------
----------------------------------------------------------------------------------------------------
SUBDIRS += deviceintegration
*/

import qbs

QtModuleProject {
    name: "QtDeviceDiscoverySupport"
    simpleName: "devicediscovery_support"
    internal: true

    QtHeaders {
    }

    QtModule {
        qbsSearchPaths: [project.qtbaseShadowDir + "/src/corelib/qbs"]

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core-private" }
        Depends { name: "QtCorePrivateConfig" }
        Depends { name: "Glib"; condition: QtCorePrivateConfig.glib }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")

        files: ["qdevicediscovery_p.h"]

        Group {
            condition: QtCorePrivateConfig.evdev
            files: [
                "qdevicediscovery_static.cpp",
                "qdevicediscovery_static_p.h",
            ]
        }
        Group {
            condition: QtCorePrivateConfig.libudev
            files: [
                "qdevicediscovery_udev.cpp",
                "qdevicediscovery_udev_p.h",
            ]
        }
        Group {
            condition: !(QtCorePrivateConfig.evdev || QtCorePrivateConfig.libudev)
            files: [
                "qdevicediscovery_dummy.cpp",
                "qdevicediscovery_dummy_p.h",
            ]
        }
    }
}

/*
QT_FOR_CONFIG += gui-private
*/

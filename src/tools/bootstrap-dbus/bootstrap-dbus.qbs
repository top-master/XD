import qbs

QtModuleProject {
    name: "QtBootstrapDBus"
    simpleName: "bootstrap_dbus"
    internal: true

    QtModule {
        condition: QtGlobalPrivateConfig.dbus
            && (QtGlobalConfig.cross_compile || QtGlobalPrivateConfig.private_tests)
        hostBuild: true
        Properties {
            condition: QtGlobalConfig.release_tools
            qbs.buildVariant: "release"
        }
        qbs.buildVariant: original

        // Ignore debug_and_release for host tools.
        multiplexByQbsProperties: base.filter(function(name) { return name !== "buildVariants"; })

        Export {
            Depends { name: "QtDBusHeaders" }
            Depends { name: "cpp" }
            cpp.includePaths: project.includePaths.concat(QtDBusHeaders.includePaths)
        }

        Depends { name: "QtGlobalConfig" }
        Depends { name: "QtGlobalPrivateConfig" }
        Depends { name: "Qt.bootstrap-private" }
        Depends { name: "QtDBusHeaders" }

        cpp.includePaths: base.concat(project.includePaths, QtDBusHeaders.includePaths)
        cpp.defines: base.concat("QT_NO_FOREACH", "QT_NO_CAST_FROM_ASCII")
        cpp.cxxFlags: base.concat(QtGlobalPrivateConfig.dbus_host_cflags)

        Group {
            prefix: "../../dbus/"
            files: [
                "qdbus_symbols.cpp",
                "qdbusargument.cpp",
                "qdbuserror.cpp",
                "qdbusextratypes.cpp",
                "qdbusintrospection.cpp",
                "qdbusmetatype.cpp",
                "qdbusmisc.cpp",
                "qdbusunixfiledescriptor.cpp",
                "qdbusutil.cpp",
                "qdbusxmlparser.cpp",
            ]
        }
    }
}

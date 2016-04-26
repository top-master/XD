import qbs
import "../../../qbs/imports/QtUtils.js" as QtUtils

QtModuleProject {
    id: root
    name: "QtBootstrapDBus"
    simpleName: "bootstrap_dbus"
    prefix: project.qtbasePrefix + "src/dbus/"
    defines: [
        "QT_BOOTSTRAPPED",
        "QT_NO_CAST_FROM_ASCII",
    ]
    includePaths: [].concat(
        QtUtils.includesForModule("QtDBus", project.buildDirectory + '/' + root.simpleName,
                                  project.version),
        QtUtils.includesForModule("QtDBus-private", project.buildDirectory + '/' + root.simpleName,
                                  project.version),
        QtUtils.includesForModule("QtCore", project.buildDirectory + "/bootstrap",
                                  project.version)
    )

    QtHeaders {
        name: root.headersName
        condition: project.crossBuild
        profiles: [project.hostProfile]
        install: false
        Depends { name: "QtBootstrapHeaders" }
        sync.module: "QtDBus"
        sync.prefix: root.simpleName
        Group {
            name: "headers"
            prefix: root.prefix
            files: [
                "*.h",
            ]
            fileTags: "hpp_syncable"
        }
    }

    QtModule {
        name: root.privateName
        condition: project.crossBuild
        profiles: project.hostProfile
        hostBuild: true
        parentName: root.name
        simpleName: root.simpleName + "_private"
        targetName: root.targetName
        type: ["staticlibrary", "prl", "pri"]
        qmakeProject: project.qtbasePrefix + "src/tools/bootstrap-dbus/bootstrap-dbus.pro"
        qmakeProjectPrefix: "bootstrap-dbus/"

        Depends { name: root.headersName }
        Depends { name: "Qt.bootstrap-private" }

        cpp.includePaths: base.concat(root.includePaths)
        cpp.defines: base.concat(root.defines)
        cpp.cxxFlags: base.concat(project.cFlagsDbusHost)

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "qdbusargument.cpp",
                "qdbuserror.cpp",
                "qdbusextratypes.cpp",
                "qdbusintrospection.cpp",
                "qdbusmetatype.cpp",
                "qdbusmisc.cpp",
                "qdbus_symbols.cpp",
                "qdbusunixfiledescriptor.cpp",
                "qdbusutil.cpp",
                "qdbusxmlparser.cpp",
            ]
        }

        Export {
            Depends { name: "cpp" }
            cpp.defines: root.defines
            cpp.includePaths: root.includePaths
            cpp.cxxLanguageVersion: "c++11"
        }
    }
}

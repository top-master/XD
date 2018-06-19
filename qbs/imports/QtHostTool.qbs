import qbs
import qbs.FileInfo

CppApplication {
    property string description     // TODO: Generate a Windows resource, and set this description.
    property string toolFileTag
    type: toolFileTag ? base.concat(toolFileTag) : base
    consoleApplication: true
    cpp.defines: ["QT_USE_QSTRINGBUILDER"]
    cpp.discardUnusedData: true

    property bool useBootstrapLib: Qt.global.config.cross_compile
    Properties {
        condition: Qt.global.config.release_tools
        qbs.buildVariant: "release"
    }
    qbs.buildVariant: original

    multiplexByQbsProperties: ["profiles"]
    property string hostProfile: "qt_hostProfile"
    qbs.profiles: [hostProfile]

    Depends { name: "Qt.global" }
    Depends { name: "Qt.bootstrap_private"; condition: useBootstrapLib }
    Depends { name: "Qt.core"; condition: !useBootstrapLib }
    Depends { name: "osversions" }
    Depends { name: "qt_common" }

    property bool createQbsModule: false
    Depends { name: "Exporter.qbs"; condition: createQbsModule }
    property string moduleInstallDir: FileInfo.joinPaths("lib", "qbs", "modules", name)
    Group {
        fileTagsFilter: ["Exporter.qbs.module"]
        qbs.install: true
        qbs.installDir: moduleInstallDir
    }

    Properties {
        condition: qbs.toolchain.contains("gcc") && Qt.global.config.rpath && !useBootstrapLib
        cpp.rpaths: cpp.rpathOrigin + "/../lib"
    }

    Group {
        fileTagsFilter: "application"
        fileTags: toolFileTag ? [toolFileTag] : undefined
        qbs.install: true
        qbs.installDir: "bin"
    }
}

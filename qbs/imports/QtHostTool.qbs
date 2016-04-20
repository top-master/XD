import qbs

CppApplication {
    profiles: project.hostProfile
    consoleApplication: true
    destinationDirectory: project.binDirectory
    cpp.defines: ["QT_USE_QSTRINGBUILDER"]

    property bool useBootstrapLib: project.crossBuild

    Depends { name: "Qt.bootstrap-private"; condition: useBootstrapLib }
    Depends { name: "Qt.core"; condition: !useBootstrapLib }
    Depends { name: "osversions" }

    Properties {
        condition: qbs.targetOS.contains("linux") && qbs.toolchain.contains("clang")
        cpp.cxxStandardLibrary: "libstdc++"
    }

    Properties {
        condition: qbs.toolchain.contains("gcc") && project.rpath && !useBootstrapLib
        cpp.rpaths: qbs.targetOS.contains("darwin") ? ["@loader_path/../lib"] : ["$ORIGIN/../lib"]
    }

    cpp.cxxLanguageVersion: "c++11"

    Group {
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}

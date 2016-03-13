import qbs

CppApplication {
    profiles: project.hostProfile
    consoleApplication: true
    destinationDirectory: project.binDirectory
    cpp.defines: ["QT_USE_QSTRINGBUILDER"]
    Depends { name: "Qt.bootstrap-private" }

    Properties {
        condition: qbs.targetOS.contains("linux") && qbs.toolchain.contains("clang")
        cpp.cxxStandardLibrary: "libstdc++"
    }

    Group {
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}

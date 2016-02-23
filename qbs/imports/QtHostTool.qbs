import qbs

CppApplication {
    profiles: project.hostProfile
    consoleApplication: true
    destinationDirectory: project.binDirectory
    cpp.defines: ["QT_USE_QSTRINGBUILDER"]
    Depends { name: "Qt.bootstrap-private" }

    Group {
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}

import qbs

QtExample {
    name: "customtype"
    condition: Qt.widgets.present
    installDir: project.examplesInstallDir + "/corelib/tools/customcompleter"
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "main.cpp",
        "message.cpp",
        "message.h",
    ]
}

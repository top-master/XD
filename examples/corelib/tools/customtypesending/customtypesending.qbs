import qbs

QtExample {
    name: "customtypesending"
    condition: Qt.widgets.present
    installDir: project.examplesInstallDir + "/corelib/tools/customcompleter"
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "main.cpp",
        "message.cpp",
        "message.h",
        "window.cpp",
        "window.h",
    ]
}

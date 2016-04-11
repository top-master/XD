import qbs

QtExample {
    name: "styleplugin"
    condition: Qt.widgets.present
    destinationDirectory: project.buildDirectory + "/examples/widgets/tools/styleplugin"
    installDir: project.examplesInstallDir + "/widgets/tools/styleplugin"

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "stylewindow.cpp",
        "stylewindow.h",
    ]
}

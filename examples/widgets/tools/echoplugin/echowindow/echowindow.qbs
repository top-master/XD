import qbs

QtExample {
    name: "echopluginwindow"
    targetName: "echoplugin"
    condition: Qt.widgets.present && !project.disabledFeatures.contains("library")
    destinationDirectory: project.buildDirectory + "/examples/widgets/tools/echoplugin"
    installDir: project.examplesInstallDir + "/widgets/tools/echoplugin"

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "echointerface.h",
        "echowindow.cpp",
        "echowindow.h",
        "main.cpp",
    ]
}

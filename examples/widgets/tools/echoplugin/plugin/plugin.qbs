import qbs

QtExample {
    name: "echoplugin"
    condition: Qt.widgets.present && !project.disabledFeatures.contains("library")
    type: ["dynamiclibrary"]
    destinationDirectory: project.buildDirectory + "/examples/widgets/tools/echoplugin/plugins"
    installDir: project.examplesInstallDir + "/widgets/tools/echoplugin/plugin"

    Depends { name: "Qt.widgets"; required: false }

    cpp.includePaths: base.concat("../echowindow")

    files: [
        "echoplugin.cpp",
        "echoplugin.h",
        "echoplugin.json",
    ]

}

// CONFIG         += plugin

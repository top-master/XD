import qbs

QtExample {
    name: "simplestyleplugin"
    type: ["dynamiclibrary"]
    condition: Qt.widgets.present
    destinationDirectory: project.buildDirectory + "/examples/widgets/tools/styleplugin/styles"
    installDir: project.examplesInstallDir + "/widgets/tools/styleplugin/styles"

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "simplestyle.cpp",
        "simplestyle.h",
        "simplestyle.json",
        "simplestyleplugin.cpp",
        "simplestyleplugin.h",
    ]
}

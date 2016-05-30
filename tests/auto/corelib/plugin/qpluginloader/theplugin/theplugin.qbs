import qbs

QtAutotestPlugin {
    name: "theplugin"
    targetName: "theplugin"
    destinationDirectory: project.buildDirectory + "/qpluginloader_test/bin"

    files: [
        "theplugin.cpp",
        "theplugin.h",
    ]
}

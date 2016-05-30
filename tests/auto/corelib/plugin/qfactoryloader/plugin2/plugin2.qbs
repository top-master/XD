import qbs

QtAutotestPlugin {
    name: "plugin2"
    destinationDirectory: project.buildDirectory + "/qfactoryloadertest/bin"

    files: [
        "plugin2.cpp",
        "plugin2.h",
    ]
}

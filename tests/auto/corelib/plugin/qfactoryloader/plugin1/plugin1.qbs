import qbs

QtAutotestPlugin {
    name: "plugin1"
    destinationDirectory: project.buildDirectory + "/qfactoryloadertest/bin"

    files: [
        "plugin1.cpp",
        "plugin1.h",
    ]
}

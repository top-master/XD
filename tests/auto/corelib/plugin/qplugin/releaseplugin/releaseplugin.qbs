import qbs

QtAutotestPlugin {
    destinationDirectory: project.buildDirectory + "/qplugin_test/plugins"
    condition: !qbs.targetOS.contains("windows") || qbs.buildVariant === "release"
    qbs.buildVariant: "release"
    files: [
        "main.cpp",
    ]
}

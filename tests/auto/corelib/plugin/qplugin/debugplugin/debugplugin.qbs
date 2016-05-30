import qbs

QtAutotestPlugin {
    destinationDirectory: project.buildDirectory + "/qplugin_test/plugins"
    condition: !qbs.targetOS.contains("windows") || qbs.buildVariant === "debug"
    qbs.buildVariant: "debug"
    files: [
        "main.cpp",
    ]
}

import qbs

QtAutotestPlugin {
    name: "almostplugin"
    condition: !qbs.targetOS.contains("android") && !qbs.targetOS.contains("windows")
               && !qbs.targetOS.contains("darwin")
    destinationDirectory: project.buildDirectory + "/qpluginloader_test/bin"

    cpp.allowUnresolvedSymbols: true

    files: [
        "almostplugin.cpp",
        "almostplugin.h",
    ]
}

import qbs

QtProduct {
    property string category

    type: project.staticBuild ? "staticlibrary" : "dynamiclibrary"
    destinationDirectory: project.buildDirectory + "/plugins/" + category
    targetName: name + (qbs.enableDebugCode && qbs.targetOS.contains("windows") ? 'd' : '')

    Group {
        fileTagsFilter: ["debuginfo", "dynamiclibrary", "staticlibrary"]
        qbs.install: true
        qbs.installSourceBase: project.buildDirectory
    }
}

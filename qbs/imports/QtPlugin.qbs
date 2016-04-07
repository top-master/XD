import qbs

QtProduct {
    property string category

    type: project.staticBuild ? "staticlibrary" : "dynamiclibrary"
    destinationDirectory: project.buildDirectory + "/plugins/" + category
    targetName: name + (qbs.enableDebugCode && qbs.targetOS.contains("windows") ? 'd' : '')
    cpp.defines: base.concat("QT_PLUGIN").concat(project.staticBuild ? ["QT_STATICPLUGIN"] : [])

    Group {
        fileTagsFilter: ["debuginfo", "dynamiclibrary", "staticlibrary"]
        qbs.install: true
        qbs.installSourceBase: project.buildDirectory
    }
}

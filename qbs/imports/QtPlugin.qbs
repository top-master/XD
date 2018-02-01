import qbs

QtProduct {
    type: [QtGlobalConfig.staticBuild ? "staticlibrary" : "dynamiclibrary"]
    property string pluginType: {
        throw new Error("QtPlugin.pluginType needs to be defined.")
    }
    property string pluginClassName: {
        throw new Error("QtPlugin.pluginClassName needs to be defined.")
    }
    Depends { name: "cpp" }
    cpp.defines: [
        "QT_NO_FOREACH"
    ]
    Group {
        fileTagsFilter: product.type
        qbs.install: !QtGlobalConfig.staticBuild
        qbs.installDir: "plugins/" + product.pluginType
        qbs.installSourceBase: product.buildDirectory
    }
}

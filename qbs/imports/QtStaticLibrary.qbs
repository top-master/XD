import qbs

QtProduct {
    type: ["staticlibrary"]
    property bool isInternal: true
    Group {
        fileTagsFilter: ["staticlibrary"]
        qbs.install: !isInternal || QtGlobalConfig.staticBuild
        qbs.installDir: "lib"
        qbs.installSourceBase: product.buildDirectory
    }
}

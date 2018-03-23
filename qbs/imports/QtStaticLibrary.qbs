import qbs

QtProduct {
    type: ["staticlibrary"]
    property bool isInternal: true
    Group {
        fileTagsFilter: ["staticlibrary"]
        qbs.install: !isInternal || Qt.global.config.staticBuild
        qbs.installDir: "lib"
        qbs.installSourceBase: product.buildDirectory
    }
}

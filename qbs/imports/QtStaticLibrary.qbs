import qbs

QtProduct {
    type: ["staticlibrary"]
    property bool isInternal: true
    property bool install: !isInternal || Qt.global.config.staticBuild
    Group {
        fileTagsFilter: ["staticlibrary"]
        qbs.install: install
        qbs.installDir: "lib"
        qbs.installSourceBase: product.buildDirectory
    }
}

import qbs

QtProduct {
    type: ["staticlibrary"]
    Depends { name: "qt_common_libs_plugins" }
    Depends { name: "qt_warnings" }
    property bool isInternal: true
    property bool install: !isInternal || Qt.global.config.staticBuild
    Group {
        fileTagsFilter: ["staticlibrary"]
        qbs.install: install
        qbs.installDir: "lib"
        qbs.installSourceBase: product.buildDirectory
    }
}

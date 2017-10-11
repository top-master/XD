import qbs

QtProduct {
    type: ["staticlibrary"]
    Group {
        fileTagsFilter: ["staticlibrary"]
        qbs.install: true
        qbs.installDir: "lib"
        qbs.installSourceBase: product.buildDirectory
    }
}

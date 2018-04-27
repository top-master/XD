import qbs

QtProduct {
    type: ["dynamiclibrary"]
    Group {
        fileTagsFilter: ["dynamiclibrary"]
        qbs.install: true
        qbs.installDir: "bin"
        qbs.installSourceBase: product.buildDirectory
    }
    Group {
        fileTagsFilter: ["dynamiclibrary_import"]
        qbs.install: true
        qbs.installDir: "lib"
        qbs.installSourceBase: product.buildDirectory
    }
}

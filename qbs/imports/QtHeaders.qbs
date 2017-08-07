import qbs
import qbs.FileInfo

Product {
    name: project.headersName
    type: ["hpp_private", "hpp_public", "hpp_forwarding", "hpp_module", "hpp_qpa", "hpp_depends", "hpp"]
    property bool install: true

    Depends { name: "sync" }
    sync.module: project.name

    Group {
        fileTagsFilter: ["hpp_public", "hpp_forwarding", "hpp_module", "hpp_depends"]
        qbs.install: product.install
        qbs.installDir: sync.prefix + '/' + sync.module
    }

    Group {
        fileTagsFilter: "hpp_private"
        qbs.install: product.install
        qbs.installDir: sync.prefix + '/' + sync.module + "/"
                        + project.version + "/" + sync.module
                        + "/private"
    }

    Group {
        fileTagsFilter: "hpp_qpa"
        qbs.install: product.install
        qbs.installDir: FileInfo.joinPaths(sync.prefix, sync.module, project.version, sync.module,
                                           "qpa")
    }

    property string baseDir: sourceDirectory
    files: [baseDir + "/*.h", baseDir + "/**/*.h"]
    excludeFiles: [baseDir + "/doc/**"]
}

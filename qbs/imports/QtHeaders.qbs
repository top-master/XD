import qbs
import qbs.FileInfo

Product {
    name: project.headersName
    type: ["hpp_private", "hpp_public", "hpp_forwarding", "hpp_module", "hpp_qpa", "hpp_depends", "hpp"]
    condition: project.conditionFunction()
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
    property stringList shadowBuildFiles: []
    files: [baseDir + "/*.h", baseDir + "/**/*.h"].concat(
               project.isShadowBuild ? shadowBuildFiles : [])
    excludeFiles: [baseDir + "/doc/**"]

    Export {
        property stringList includePaths: project.includePaths
        property stringList publicIncludePaths: project.publicIncludePaths
    }
}

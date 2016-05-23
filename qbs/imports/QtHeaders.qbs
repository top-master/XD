import qbs
import qbs.FileInfo

Product {
    type: ["hpp_private", "hpp_public", "hpp_forwarding", "hpp_module", "hpp_qpa", "hpp_depends"]
    property bool install: project.buildForVariant

    profiles: project.targetProfiles

    Depends { name: "sync" }

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
}

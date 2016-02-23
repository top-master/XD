import qbs

Product {
    type: "hpp"
    property bool install: project.buildForVariant

    profiles: project.targetProfiles

    Depends { name: "sync" }

    Group {
        fileTagsFilter: ["hpp_public", "hpp_" + sync.module]
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
        qbs.installDir: sync.prefix + "/QtGui/" + project.version + "/QtGui/qpa"
    }
}

import qbs

QtAutotestProject {
    name: "corelib_plugin_tests"
    condition: !qbs.targetOS.contains("ios")
    qbsSearchPaths: ["."]
    references: [
        "qfactoryloader",
        "qlibrary",
        "qplugin/qplugin.qbs",
        "qpluginloader",
        "quuid",
    ]
}

import qbs

Project {
    condition: !project.disabledFeatures.contains("library")
    references: [
        "debugplugin",
        "releaseplugin",
        "tst_qplugin.qbs",
    ]
}

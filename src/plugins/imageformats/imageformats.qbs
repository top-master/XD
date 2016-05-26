import qbs

Project {
    condition: project.gui && !project.disabledFeatures.contains("imageformatplugin")
    references: [
        "gif",
        "ico",
        "jpeg",
    ]
}

import qbs

Project {
    condition: !project.disabledFeatures.contains("library")
    references: [
        "lib",
        "lib2",
        "tst",
    ]
}

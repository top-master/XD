import qbs

Project {
    condition: !project.disabledFeatures.contains("library")
    references: [
        "almostplugin",
        "lib",
        "theplugin",
        "tst",
    ]

    SubProject {
        Properties {
            condition: qbs.targetOS.contains("darwin") && config.contains("private_tests")
        }
        filePath: "machtest/machtest.qbs"
    }
}

import qbs

Project {
    name: "qobject_test"
    condition: project.config.contains("testcase_targets") && project.network
    references: ["signalbug/signalbug.qbs", "test"]
}

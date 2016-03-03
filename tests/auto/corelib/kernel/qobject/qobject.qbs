import qbs

Project {
    name: "qobject_test"
    condition: project.network
    references: ["signalbug/signalbug.qbs", "test"]
}

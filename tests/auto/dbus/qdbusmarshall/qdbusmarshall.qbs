import qbs

Project {
    condition: project.private_tests
    references: [
        "qdbusmarshall",
        "qpong",
    ]
}

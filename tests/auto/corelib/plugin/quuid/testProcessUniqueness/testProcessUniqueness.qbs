import qbs

QtProduct {
    type: ["application"]
    destinationDirectory: project.buildDirectory + "/quuid_test/testProcessUniqueness"
    consoleApplication: true

    Depends { name: "Qt.core" }

    files: [
        "main.cpp",
    ]
}

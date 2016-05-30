import qbs

QtProduct {
    type: ["application"]
    destinationDirectory: project.buildDirectory + "/qtextstream_test/stdinProcess"
    consoleApplication: true
    Depends { name: "Qt.core" }
    files: [
        "main.cpp",
    ]
}

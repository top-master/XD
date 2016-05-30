import qbs

QtProduct {
    type: ["application"]
    destinationDirectory: project.buildDirectory + "/qtextstream_test/readAllStdinProcess"
    consoleApplication: true
    Depends { name: "Qt.core" }
    files: [
        "main.cpp",
    ]
}

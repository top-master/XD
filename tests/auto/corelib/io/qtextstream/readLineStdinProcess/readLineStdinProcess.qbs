import qbs

QtProduct {
    type: ["application"]
    destinationDirectory: project.buildDirectory + "/qtextstream_test/readLineStdinProcess"
    consoleApplication: true
    Depends { name: "Qt.core" }
    files: [
        "main.cpp",
    ]
}

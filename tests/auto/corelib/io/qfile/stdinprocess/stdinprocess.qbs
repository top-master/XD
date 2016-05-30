import qbs

QtProduct {
    name: "stdinprocess"
    type: ["application"]
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qfile_test/stdinprocess"
    Depends { name: "Qt.core" }
    files: [
        "main.cpp",
    ]
}

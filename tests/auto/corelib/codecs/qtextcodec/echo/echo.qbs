import qbs

QtProduct {
    type: ["application"]
    destinationDirectory: project.buildDirectory + "/qtextcodectest/echo"
    consoleApplication: true
    Depends { name: "Qt.core" }
    files: [
        "main.cpp",
    ]
}

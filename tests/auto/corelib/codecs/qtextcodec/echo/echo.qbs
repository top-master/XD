import qbs

QtAutotestHelperApp {
    destinationDirectory: project.buildDirectory + "/qtextcodectest/echo"
    Depends { name: "Qt.core" }
    files: [
        "main.cpp",
    ]
}

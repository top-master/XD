import qbs

QtProduct {
    type: ["application"]
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qcommandlineparser_test/testhelper"
    Depends { name: "Qt.core" }
    files: [
        "qcommandlineparser_test_helper.cpp",
    ]
}

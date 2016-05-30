import qbs

QtAutotest {
    name: "tst_qcommandlineparser"
    destinationDirectory: project.buildDirectory + "/qcommandlineparser_test"
    Depends { name: "qcommandlineparser_test_helper" }
    files: [
        "tst_qcommandlineparser.cpp",
    ]
}

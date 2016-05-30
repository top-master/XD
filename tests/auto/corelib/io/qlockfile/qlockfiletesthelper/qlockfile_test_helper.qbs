import qbs

QtProduct {
    type: ["application"]
    name: "qlockfile_test_helper"
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qlockfiletest/qlockfiletesthelper"
    Depends { name: "Qt.core" }
    files: [
        "qlockfile_test_helper.cpp",
    ]
}

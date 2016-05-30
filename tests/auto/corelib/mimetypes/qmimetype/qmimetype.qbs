import qbs

QtAutotest {
    name: "tst_qmimetype"
    condition: project.config.contains("private_tests")
    Depends { name: "Qt.core-private"; condition: project.config.contains("private_tests") }
    files: [
        "tst_qmimetype.cpp",
    ]
}

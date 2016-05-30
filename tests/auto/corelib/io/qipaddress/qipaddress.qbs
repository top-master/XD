import qbs

QtAutotest {
    name: "tst_qipaddress"
    condition: project.config.contains("private_tests")
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qipaddress.cpp",
    ]
}

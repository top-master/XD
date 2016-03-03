import qbs

QtAutotest {
    name: "tst_qcoreapplication"
    condition: project.private_tests
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qcoreapplication.cpp",
        "tst_qcoreapplication.h",
    ]
}

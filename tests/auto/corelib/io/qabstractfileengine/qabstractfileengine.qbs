import qbs

QtAutotest {
    name: "tst_qabstractfileengine"
    condition: project.config.contains("private_tests")
    Depends { name: "Qt.core-private" }
    files: [
        "qabstractfileengine.qrc",
        "resources/*",
        "tst_qabstractfileengine.cpp",
    ]
}

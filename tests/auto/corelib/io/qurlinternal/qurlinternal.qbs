import qbs

QtAutotest {
    name: "tst_qurlinternal"
    condition: project.config.contains("private_tests")
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qurlinternal.cpp",
        "../../codecs/utf8/utf8data.cpp",
    ]
}

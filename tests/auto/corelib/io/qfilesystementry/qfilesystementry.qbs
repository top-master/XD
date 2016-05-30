import qbs

QtAutotest {
    name: "tst_qfilesystementry"
    condition: !qbs.targetOS.contains("windows") || project.config.contains("private_tests")
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qfilesystementry.cpp",
    ]
    Group {
        name: "corelib_sources"
        prefix: project.qtbasePrefix + "src/corelib/io/"
        files: [
            "qfilesystementry.cpp",
            "qfilesystementry_p.h",
        ]
    }
}

import qbs

QtAutotest {
    name: "tst_qfilesystementry"
    condition: base && !qbs.targetOS.contains("windows") || Qt.global.privateConfig.private_tests

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.global" }
    files: "tst_qfilesystementry.cpp"
    Group {
        name: "corelib_sources"
        prefix: project.qtbaseDir + "/src/corelib/io/"
        files: [
            "qfilesystementry.cpp",
            "qfilesystementry_p.h",
        ]
    }
}

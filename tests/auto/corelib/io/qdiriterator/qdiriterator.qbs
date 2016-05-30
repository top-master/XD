import qbs

QtAutotest {
    name: "tst_qdiriterator"
    Depends { name: "Qt.core-private" }
    Properties {
        condition: project.config.contains("builtin_testdata")
        cpp.defines: base.concat(["BUILTIN_TESTDATA"])
    }
    files: [
        "qdiriterator.qrc",
        "tst_qdiriterator.cpp",
    ]
}

// TESTDATA += entrylist
// win32: CONFIG += insignificant_test # Crashes on Windows in release builds

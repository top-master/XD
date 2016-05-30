import qbs

QtAutotest {
    name: "tst_qstringmatcher"
    cpp.defines: base.concat(["QT_NO_CAST_TO_ASCII"])
    files: [
        "tst_qstringmatcher.cpp",
    ]
}

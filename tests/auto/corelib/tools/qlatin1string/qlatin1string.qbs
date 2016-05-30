import qbs

QtAutotest {
    name: "tst_qlatin1string"
    cpp.defines: base.concat(["QT_NO_CAST_TO_ASCII", "QT_DISABLE_DEPRECATED_BEFORE=0"])
    files: [
        "tst_qlatin1string.cpp",
    ]
}

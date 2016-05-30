import qbs

QtAutotest {
    name: "tst_qcollator"

    cpp.defines: base.concat(["QT_NO_CAST_TO_ASCII"]).concat(project.icu ? ["QT_USE_ICU"] : [])

    files: [
        "tst_qcollator.cpp",
    ]
}

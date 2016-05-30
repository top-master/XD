import qbs

QtAutotest {
    name: "tst_qabstractitemmodel"

    Depends { name: "modeltestlib" }

    cpp.defines: base.concat(["QT_DISABLE_DEPRECATED_BEFORE=0"])

    files: [
        "tst_qabstractitemmodel.cpp",
    ]
}

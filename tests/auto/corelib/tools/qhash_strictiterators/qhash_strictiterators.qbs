import qbs

QtAutotest {
    name: "tst_qhash_strictiterators"
    cpp.defines: base.concat(["QT_STRICT_ITERATORS", "tst_QHash=tst_QHash_StrictIterators"])
    files: [
        "../qhash/tst_qhash.cpp",
    ]
}

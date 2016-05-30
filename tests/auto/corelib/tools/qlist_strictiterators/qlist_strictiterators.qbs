import qbs

QtAutotest {
    name: "tst_qlist_strictiterators"
    cpp.defines: base.concat(["QT_STRICT_ITERATORS", "tst_QList=tst_QList_StrictIterators"])
    files: [
        "../qlist/tst_qlist.cpp",
    ]
}

import qbs

QtAutotest {
    name: "tst_qmap_strictiterators"
    cpp.defines: base.concat(["QT_STRICT_ITERATORS", "tst_QMap=tst_QMap_StrictIterators"])
    files: [
        "../qmap/tst_qmap.cpp",
    ]
}

import qbs

QtAutotest {
    name: "tst_qvector_strictiterators"
    cpp.defines: base.concat(["QT_STRICT_ITERATORS=1",  "tst_QVector=tst_QVector_StrictIterators"])
    files: [
        "../qvector/tst_qvector.cpp",
    ]
}

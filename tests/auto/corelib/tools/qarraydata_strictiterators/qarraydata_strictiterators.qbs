import qbs

QtAutotest {
    name: "tst_qarraydata_strictiterators"
    cpp.defines: base.concat([
            "QT_STRICT_ITERATORS=1",
            "tst_QArrayData=tst_QArrayData_StrictIterators",
    ])
    files: [
        "../qarraydata/simplevector.h",
        "../qarraydata/tst_qarraydata.cpp",
    ]
}

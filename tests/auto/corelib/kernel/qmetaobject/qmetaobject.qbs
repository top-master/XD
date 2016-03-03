import qbs

QtAutotest {
    name: "tst_qmetaobject"
    Depends { name: "Qt.core-private" }
    files: ["tst_qmetaobject.cpp"]
}

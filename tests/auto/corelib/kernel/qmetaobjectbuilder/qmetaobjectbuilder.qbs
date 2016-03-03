import qbs

QtAutotest {
    name: "tst_qmetaobjectbuilder"
    Depends { name: "Qt.core-private" }
    files: "tst_qmetaobjectbuilder.cpp"
}


import qbs

QtAutotest {
    name: "tst_qmetaobject"
    supportsUiKit: true
    Depends { name: "Qt.core_private" }
    files: "tst_qmetaobject.cpp"
}

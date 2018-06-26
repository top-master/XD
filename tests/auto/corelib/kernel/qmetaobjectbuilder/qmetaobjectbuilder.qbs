import qbs

QtAutotest {
    name: "tst_qmetaobjectbuilder"
    supportsUiKit: true
    Depends { name: "Qt.core_private" }
    files: "tst_qmetaobjectbuilder.cpp"
}


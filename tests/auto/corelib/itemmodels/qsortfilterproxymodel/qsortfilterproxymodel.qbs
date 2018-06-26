import qbs

QtAutotest {
    name: "tst_qsortfilterproxymodel"
    condition: base && Qt.global.privateConfig.widgets

    Depends { name: "Qt.widgets" }
    Depends { name: "dynamictreemodel" }

    files: "tst_qsortfilterproxymodel.cpp"
}

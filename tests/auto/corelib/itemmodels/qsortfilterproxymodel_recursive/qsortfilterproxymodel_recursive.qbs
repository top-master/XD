import qbs

QtAutotest {
    name: "tst_qsortfilterproxymodel_recursive"
    condition: base && Qt.global.privateConfig.gui

    Depends { name: "Qt.gui" }

    files: "tst_qsortfilterproxymodel_recursive.cpp"
}

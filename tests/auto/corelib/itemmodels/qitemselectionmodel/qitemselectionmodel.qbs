import qbs

QtAutotest {
    name: "tst_qitemselectionmodel"
    condition: base && Qt.global.privateConfig.gui

    Depends { name: "Qt.gui" }

    files: "tst_qitemselectionmodel.cpp"
}

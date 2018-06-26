import qbs

QtAutotest {
    name: "tst_qmimedata"
    condition: base && Qt.global.privateConfig.gui
    supportsUiKit: true
    Depends { name: "Qt.gui" }
    files: "tst_qmimedata.cpp"
}

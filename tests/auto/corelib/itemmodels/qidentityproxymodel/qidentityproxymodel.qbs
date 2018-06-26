import qbs

QtAutotest {
    name: "tst_qidentityproxymodel"
    condition: base && Qt.global.privateConfig.gui

    Depends { name: "Qt.gui" }
    Depends { name: "dynamictreemodel" }

    files: "tst_qidentityproxymodel.cpp"
}

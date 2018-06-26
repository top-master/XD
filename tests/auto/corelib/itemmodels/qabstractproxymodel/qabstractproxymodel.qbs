import qbs

QtAutotest {
    name: "tst_qabstractproxymodel"
    condition: base && Qt.global.privateConfig.gui

    Depends { name: "Qt.gui" }

    cpp.defines: base.concat("QT_DISABLE_DEPRECATED_BEFORE=0")

    files: "tst_qabstractproxymodel.cpp"
}

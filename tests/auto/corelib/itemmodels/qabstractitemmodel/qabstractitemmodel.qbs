import qbs

QtAutotest {
    name: "tst_qabstractitemmodel"
    condition: base && Qt.global.privateConfig.gui

    Depends { name: "Qt.gui" }
    Depends { name: "dynamictreemodel" }

    cpp.defines: base.concat("QT_DISABLE_DEPRECATED_BEFORE=0")

    files: "tst_qabstractitemmodel.cpp"
}

import qbs

QtAutotest {
    name: "tst_qitemmodel"
    condition: base && Qt.global.privateConfig.widgets && Qt.global.privateConfig.sql

    Depends { name: "Qt.sql" }
    Depends { name: "Qt.widgets" }

    files: "tst_qitemmodel.cpp"
}

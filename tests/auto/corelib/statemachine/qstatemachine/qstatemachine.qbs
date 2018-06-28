import qbs

QtAutotest {
    name: "tst_qstatemachine"

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.widgets"; condition: Qt.global.privateConfig.widgets }

    files: "tst_qstatemachine.cpp"
}

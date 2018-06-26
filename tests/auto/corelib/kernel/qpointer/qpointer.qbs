import qbs

QtAutotest {
    name: "tst_qpointer"
    supportsUiKit: true

    Depends { name: "Qt.global" }
    Depends { name: "Qt.widgets"; condition: Qt.global.privateConfig.widgets }

    files: "tst_qpointer.cpp"
}

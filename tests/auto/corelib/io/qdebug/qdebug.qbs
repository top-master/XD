import qbs

QtAutotest {
    name: "tst_qdebug"
    condition: Qt.global.config.concurrent
    Depends { name: "Qt.concurrent" }
    Depends { name: "Qt.global" }
    files: "tst_qdebug.cpp"
}

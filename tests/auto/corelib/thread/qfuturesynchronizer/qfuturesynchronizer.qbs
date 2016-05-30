import qbs

QtAutotest {
    name: "tst_qfuturesynchronizer"

    Depends { name: "Qt.concurrent" }

    files: [
        "tst_qfuturesynchronizer.cpp",
    ]
}

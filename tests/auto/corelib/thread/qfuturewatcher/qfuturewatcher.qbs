import qbs

QtAutotest {
    name: "tst_qfuturewatcher"

    Depends { name: "Qt.concurrent" }
    Depends { name: "Qt.core-private" }

    files: [
        "tst_qfuturewatcher.cpp",
    ]
}

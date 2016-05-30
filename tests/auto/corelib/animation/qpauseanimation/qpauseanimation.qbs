import qbs

QtAutotest {
    name: "tst_qpauseanimation"
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qpauseanimation.cpp",
    ]
}

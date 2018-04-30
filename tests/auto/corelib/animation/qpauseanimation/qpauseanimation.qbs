import qbs

QtAutotest {
    name: "tst_qpauseanimation"
    Depends { name: "Qt.core_private" }
    files: [
        "tst_qpauseanimation.cpp",
    ]
}

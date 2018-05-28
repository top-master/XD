import qbs

QtAutotest {
    name: "tst_qfileselectors"
    Depends { name: "Qt.core_private" }
    files: [
        "qfileselector.qrc",
        "tst_qfileselector.cpp",
    ]
}

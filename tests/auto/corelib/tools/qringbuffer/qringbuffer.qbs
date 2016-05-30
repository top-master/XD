import qbs

QtAutotest {
    name: "tst_qringbuffer"

    Depends { name: "Qt.core-private" }

    files: [
        "tst_qringbuffer.cpp",
    ]
}

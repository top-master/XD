import qbs

QtAutotest {
    name: "tst_qfuture"

    Depends { name: "Qt.concurrent" }
    Depends { name: "Qt.core-private" }

    cpp.defines: base.concat(["QT_STRICT_ITERATORS"])

    files: [
        "tst_qfuture.cpp",
    ]
}

import qbs

QtAutotest {
    name: "tst_qurlquery"
    Depends { name: "Qt.core-private" }
    cpp.defines: base.concat(['SRCDIR="' + path + '"'])
    files: [
        "tst_qurlquery.cpp",
    ]
}

import qbs

QtAutotest {
    name: "tst_qsavefile"
    Group {
        name: "source and resource"
        files: "tst_qsavefile.cpp"
        fileTags: "qt.testdata"
        overrideTags: false
    }
}

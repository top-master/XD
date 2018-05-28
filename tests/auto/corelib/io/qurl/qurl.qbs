import qbs

QtAutotest {
    name: "tst_qurl"
    Depends { name: "Qt.concurrent" }
    files: "tst_qurl.cpp"
    Group {
        name: "mac sources"
        condition: qbs.targetOS.contains("darwin")
        files: "tst_qurl_mac.mm"
    }
}

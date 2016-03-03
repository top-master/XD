import qbs

QtAutotest {
    name: "tst_qpointer"
    Depends { condition: project.widgets; name: "Qt.widgets" }
    files: "tst_qpointer.cpp"
}

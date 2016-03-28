import qbs

QtAutotest {
    name: "tst_qmimedata"
    condition: project.config.contains("testcase_targets") && project.gui
    Depends { name: "Qt.gui"; condition: project.gui; }
    files: "tst_qmimedata.cpp"
}

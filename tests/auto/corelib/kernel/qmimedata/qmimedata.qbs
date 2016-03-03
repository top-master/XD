import qbs

QtAutotest {
    name: "tst_qmimedata"
    condition: project.gui
    Depends { name: "Qt.gui"; condition: project.gui; }
    files: "tst_qmimedata.cpp"
}

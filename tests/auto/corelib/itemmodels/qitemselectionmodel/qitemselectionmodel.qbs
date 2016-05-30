import qbs

QtAutotest {
    name: "tst_qitemselectionmodel"
    condition: project.gui

    Depends { name: "Qt.gui"; condition: project.gui }

    files: [
        "tst_qitemselectionmodel.cpp",
    ]
}

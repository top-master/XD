import qbs

QtAutotest {
    name: "tst_qsortfilterproxymodel"
    condition: project.widgets

    Depends { name: "Qt.widgets"; condition: project.widgets }
    Depends { name: "modeltestlib" }

    files: [
        "tst_qsortfilterproxymodel.cpp",
    ]
}

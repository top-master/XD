import qbs

QtAutotest {
    name: "tst_modeltest"
    condition: project.widgets

    Depends { name: "Qt.widgets"; condition: project.widgets }
    Depends { name: "modeltestlib" }

    files: [
        "tst_modeltest.cpp",
    ]
}

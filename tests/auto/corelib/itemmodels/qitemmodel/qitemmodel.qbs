import qbs

QtAutotest {
    name: "tst_qitemmodel"
    condition: project.sql && project.widgets

    Depends { name: "Qt.sql"; condition: project.sql }
    Depends { name: "Qt.widgets"; condition: project.widgets }

    files: [
        "tst_qitemmodel.cpp",
    ]
}

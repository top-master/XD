import qbs

QtAutotest {
    name: "tst_qidentityproxymodel"
    condition: project.gui

    Depends { name: "Qt.gui"; condition: project.gui }
    Depends { name: "modeltestlib" }

    files: [
        "tst_qidentityproxymodel.cpp",
    ]
}

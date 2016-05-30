import qbs

QtAutotest {
    name: "tst_qpropertyanimation"
    condition: project.widgets
    Depends { name: "Qt.widgets"; condition: project.widgets.present }
    files: [
        "tst_qpropertyanimation.cpp",
    ]
}

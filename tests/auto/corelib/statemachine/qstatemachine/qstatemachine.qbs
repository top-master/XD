import qbs

QtAutotest {
    name: "tst_qstatemachine"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.widgets"; condition: project.widgets }

    files: [
        "tst_qstatemachine.cpp",
    ]
}

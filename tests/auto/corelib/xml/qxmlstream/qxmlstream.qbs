import qbs

QtAutotest {
    name: "tst_qxmlstream"
    condition: project.network

    Depends { name: "Qt.network"; condition: project.network }
    Depends { name: "Qt.xml" }

    files: [
        "tst_qxmlstream.cpp",
    ]
}

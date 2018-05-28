import qbs

QtAutotest {
    name: "tst_qabstractfileengine"
    condition: Qt.global.privateConfig.private_tests
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.global" }
    files: [
        "qabstractfileengine.qrc",
        "resources/*",
        "tst_qabstractfileengine.cpp",
    ]
}

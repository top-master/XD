import qbs

QtAutotest {
    name: "tst_qcoreapplication"
    condition: base && Qt.global.privateConfig.private_tests
    supportsUiKit: true
    version: qbs.targetOS.contains("windows") ? "1.2.3.4" : "1.2.3"

    Depends { name: "Qt.global" }
    Depends { name: "Qt.core_private" }

    files: [
        "Info.plist",
        "tst_qcoreapplication.cpp",
        "tst_qcoreapplication.h",
    ]
}

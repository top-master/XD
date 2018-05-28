import qbs

QtAutotest {
    name: "tst_qfileinfo"
    condition: Qt.global.privateConfig.private_tests
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.global" }
    Properties {
        condition: qbs.targetOS.contains("windows") && !targetsUWP
        cpp.dynamicLibraries: base.concat(["advapi32", "netapi32"])
    }

    files: [
        "qfileinfo.qrc",
        "testdata.qrc",
        "tst_qfileinfo.cpp",
    ]
}

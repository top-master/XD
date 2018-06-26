import qbs

Project {
    references: "sharedmemoryhelper"

    QtAutotest {
        name: "tst_qsharedmemory"
        condition: base && Qt.core.config.sharedmemory && Qt.global.privateConfig.private_tests
            && !qbs.targetOS.contains("android")
        Depends { name: "Qt.core_private" }
        Properties {
            condition: qbs.targetOS.contains("linux")
            cpp.dynamicLibraries: base.concat("rt")
        }
        files: "test/tst_qsharedmemory.cpp" }
}

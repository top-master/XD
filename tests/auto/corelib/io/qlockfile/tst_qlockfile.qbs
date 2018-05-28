import qbs

QtAutotest {
    name: "tst_qlockfile"
    condition: base && Qt.global.config.concurrent
    consoleApplication: true
    Depends { name: "Qt.concurrent" }
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.global" }
    Properties {
        condition: qbs.targetOS.contains("windows") && !targetsUWP
        cpp.dynamicLibraries: base.concat(["advapi32"])
    }

    files: "tst_qlockfile.cpp"
}

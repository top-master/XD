import qbs

QtAutotest {
    name: "tst_qthread"

    Properties {
        condition: qbs.targetOS.contains("linux") && !qbs.targetOS.contains("android")
        cpp.dynamicLibraries: base.concat("pthread")
    }

    files: [
        "tst_qthread.cpp",
    ]
}

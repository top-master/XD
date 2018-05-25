import qbs

QtAutotest {
    name: "tst_qnumeric"
    Depends { name: "Qt.core_private" }
    files: [
        "tst_qnumeric.cpp",
    ]

    Properties {
        condition: qbs.toolchain.contains("icc") && qbs.targetOS.contains("unix")
        cpp.cxxFlags: base.concat(["-fp-model strict"])
    }
    Properties {
        condition: qbs.toolchain.contains("icc") && qbs.targetOS.contains("windows")
        cpp.cxxFlags: base.concat(["/fp:strict"])
    }
}

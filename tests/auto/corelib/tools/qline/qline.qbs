import qbs

QtAutotest {
    name: "tst_qline"

    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("vxworks")
                   && !qbs.targetOS.contains("haiku")
        cpp.dynamicLibraries: base.concat(["m"])
    }

    files: [
        "tst_qline.cpp",
    ]
}

import qbs

QtAutotest {
    name: "tst_qsharedpointer"

    cpp.defines: {
        var defines = base.concat(["QT_DISABLE_DEPRECATED_BEFORE=0"]);
        defines.push('DEFAULT_MAKESPEC="' + project.targetMkspec + '"');
        if (project.crossBuild)
            defines.push("QTEST_NO_RTTI", "QTEST_CROSS_COMPILED");
        return defines;
    }

    files: [
        "externaltests.cpp",
        "externaltests.h",
        "forwarddeclared.cpp",
        "forwarddeclared.h",
        "nontracked.cpp",
        "nontracked.h",
        "tst_qsharedpointer.cpp",
        "wrapper.cpp",
        "wrapper.h",
    ]
}

// TESTDATA += forwarddeclared.cpp forwarddeclared.h

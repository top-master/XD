import qbs

QtAutotest {
    name: "tst_qregularexpression_alwaysoptimize"
    condition: project.config.contains("private_tests")
    files: [
        "../tst_qregularexpression.cpp",
        "../tst_qregularexpression.h",
        "tst_qregularexpression_alwaysoptimize.cpp",
    ]
}

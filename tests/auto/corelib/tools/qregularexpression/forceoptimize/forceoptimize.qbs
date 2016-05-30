import qbs

QtAutotest {
    name: "tst_qregularexpression_forceoptimize"
    cpp.defines: base.concat(["forceOptimize=true"])
    files: [
        "../tst_qregularexpression.cpp",
        "../tst_qregularexpression.h",
        "tst_qregularexpression_forceoptimize.cpp",
    ]
}

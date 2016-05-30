import qbs
import qbs.FileInfo

QtAutotest {
    targetName: "tst_qatomicinteger_" + name

    cpp.defines: base.concat([
            "QATOMIC_TEST_TYPE=" + name,
            "tst_QAtomicIntegerXX=tst_QAtomicInteger_" + name
    ])

    files: ["../tst_qatomicinteger.cpp"]
}

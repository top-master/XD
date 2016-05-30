import qbs

QtAutotest {
    name: "tst_qprocessnoapplication"
    condition: !qbs.targetOS.contains("winrt")
    files: [
        "tst_qprocessnoapplication.cpp",
    ]
}

import qbs

QtAutotest {
    name: "tst_qprocessenvironment"
    condition: !qbs.targetOS.contains("winrt")
    files: [
        "tst_qprocessenvironment.cpp",
    ]
}

import qbs

QtAutotest {
    name: "tst_qstorageinfo"
    condition: !qbs.targetOS.contains("winrt") && core_private.present
    Depends { id: core_private; name: "Qt.core-private"; required: false }
    files: [
        "tst_qstorageinfo.cpp",
    ]
}

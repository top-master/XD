import qbs

QtAutotest {
    name: "tst_qwinoverlappedionotifier"
    condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qwinoverlappedionotifier.cpp",
    ]
}

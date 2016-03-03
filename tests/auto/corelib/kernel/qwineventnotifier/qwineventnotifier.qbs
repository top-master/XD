import qbs

QtAutotest {
    name: "tst_qwineventnotifier"
    condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")
    Depends { name: "Qt.core-private" }
    files: "tst_qwineventnotifier.cpp"
}


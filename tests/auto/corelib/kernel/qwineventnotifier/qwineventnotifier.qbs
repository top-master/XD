import qbs

QtAutotest {
    name: "tst_qwineventnotifier"
    condition: base && qbs.targetOS.contains("windows") && !targetsUWP
    Depends { name: "Qt.core_private" }
    files: "tst_qwineventnotifier.cpp"
}


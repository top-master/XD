import qbs

QtAutotest {
    name: "tst_qobject"
    destinationDirectory: project.buildDirectory + "/qobjecttest/"
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network" }
    files: "../tst_qobject.cpp"
    cpp.defines: base.concat("QT_DISABLE_DEPRECATED_BEFORE=0")

    // !winrt: TEST_HELPER_INSTALLS = ../signalbug/signalbug
}

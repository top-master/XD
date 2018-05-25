import qbs

QtAutotest {
    name: "tst_qlogging"
    destinationDirectory: project.buildDirectory + "/qloggingtest"
    consoleApplication: "true"
    cpp.defines: base.concat(["QT_MESSAGELOGCONTEXT", "QT_DISABLE_DEPRECATED_BEFORE=0"])
    files: [
        "../tst_qlogging.cpp",
    ]
}
// !winrt: TEST_HELPER_INSTALLS = ../app/app
// CONFIG -= debug_and_release_target

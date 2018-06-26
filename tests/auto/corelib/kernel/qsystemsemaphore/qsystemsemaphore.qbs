import qbs

Project {
    references: "systemsemaphorehelper"

    QtAutotest {
        name: "tst_qsystemsemaphore"
        condition: base && !qbs.targetOS.contains("android") && Qt.core.config.systemsemaphore
        consoleApplication: true
        files: "test/tst_qsystemsemaphore.cpp"
    }
}

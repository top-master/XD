import qbs

QtAutotest {
    name: "tst_qsystemsemaphore"
    files: "tst_qsystemsemaphore.cpp"
    destinationDirectory: project.buildDirectory + "/systemsemaphoretest"
}

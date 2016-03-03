import qbs

QtAutotest {
    name: "tst_qsharedmemory"
    destinationDirectory: project.buildDirectory + "/qsharedmemorytest"
    Depends { name: "Qt.core-private" }

    Properties {
        condition: qbs.targetOS.contains("linux")
        cpp.dynamicLibraries: base.concat("rt")
    }
    files: "tst_qsharedmemory.cpp"
}

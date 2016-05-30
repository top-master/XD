import qbs

QtAutotest {
    name: "tst_qthreadstorage"
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qthreadstorage_test"

    Properties {
        condition: qbs.targetOS.contains("linux") && !qbs.targetOS.contains("android")
        cpp.dynamicLibraries: base.concat("pthread")
    }

    files: [
        "../tst_qthreadstorage.cpp",
    ]
}

// !winrt: TEST_HELPER_INSTALLS = ../crashonexit/crashonexit


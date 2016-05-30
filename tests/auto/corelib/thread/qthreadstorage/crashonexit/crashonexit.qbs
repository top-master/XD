import qbs

QtProduct {
    condition: !qbs.targetOS.contains("winrt")
    type: ["application"]
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qthreadstorage_test/crashonexit"
    Depends { name: "Qt.core" }
    files: [
        "crashOnExit.cpp"
    ]
}

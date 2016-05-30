import qbs

QtProduct {
    condition: !qbs.targetOS.contains("winrt")
    type: ["application"]
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qlocale_test/syslocaleapp"

    Depends { name: "Qt.core" }

    files: [
        "syslocaleapp.cpp",
    ]
}

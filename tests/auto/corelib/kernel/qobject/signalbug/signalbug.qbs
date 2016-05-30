import qbs

QtProduct {
    type: ["application"]
    name: "signalbug"
    condition: !qbs.targetOS.contains("winrt")
    consoleApplication: true
    profiles: project.targetProfiles
    destinationDirectory: project.buildDirectory + "/qobjecttest/signalbug"
    Depends { name: "Qt.core" }
    files: [
        "signalbug.cpp",
        "signalbug.h",
    ]


// # This app is testdata for tst_qobject
// target.path = $$[QT_INSTALL_TESTS]/tst_qobject/$$TARGET
// INSTALLS += target

}

import qbs

QtAutotestHelperApp {
    name: "signalbug"
    condition: !targetsUWP
    consoleApplication: true

    Depends { name: "Qt.core" }

    files: [
        "signalbug.cpp",
        "signalbug.h",
    ]
}

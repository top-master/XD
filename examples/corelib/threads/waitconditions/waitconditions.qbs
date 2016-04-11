import qbs

QtExample {
    name: "waitconditions"
    consoleApplication: true
    Depends { name: "Qt.core" }
    files: [
        "waitconditions.cpp",
    ]
}

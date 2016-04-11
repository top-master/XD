import qbs

QtExample {
    name: "semaphores"
    consoleApplication: true
    Depends { name: "Qt.core" }
    files: [
        "semaphores.cpp",
    ]
}

import qbs

QtAutotestHelperApp {
    name: "stdinprocess"
    consoleApplication: true
    Depends { name: "Qt.core" }
    files: "main.cpp"
}

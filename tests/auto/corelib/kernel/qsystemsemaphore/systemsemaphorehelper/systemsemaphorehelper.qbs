import qbs

QtAutotestHelperApp {
    targetName: "helperbinary"
    consoleApplication: true
    installSuffix: ""

    Depends { name: "Qt.core" }
    Depends { name: "Qt.testlib" }
    Depends { name: "osversions" }

    files: "main.cpp"
}

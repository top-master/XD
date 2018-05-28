import qbs

QtAutotestHelperApp {
    name: "two space s"
    installSuffix: "testProcessSpacesArgs"
    Depends { name: "Qt.core" }
    files: "main.cpp"
}

import qbs

QtAutotestHelperApp {
    installSuffix: "test Space In Name"
    Depends { name: "Qt.core" }
    files: "main.cpp"
}

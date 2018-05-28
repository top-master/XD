import qbs

QtAutotestHelperApp {
    name: "qlockfile_test_helper"
    consoleApplication: true
    installSuffix: "qlockfiletesthelper"
    Depends { name: "Qt.core" }
    files: "qlockfile_test_helper.cpp"
}

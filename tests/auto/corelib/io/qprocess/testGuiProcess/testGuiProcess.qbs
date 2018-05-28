import qbs

QtAutotestHelperApp {
    condition: base && Qt.global.privateConfig.widgets

    Depends { name: "Qt.global" }
    Depends { name: "Qt.widgets" }

    files: "main.cpp"
}

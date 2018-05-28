import qbs

QtAutotest {
    name: "tst_qprocessenvironment"
    condition: base && Qt.core.config.processenvironment
    files: "tst_qprocessenvironment.cpp"
}

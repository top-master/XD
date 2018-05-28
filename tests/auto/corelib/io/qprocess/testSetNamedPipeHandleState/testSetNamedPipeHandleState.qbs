import qbs

QtAutotestHelperApp {
    condition: base && qbs.targetOS.contains("windows")
    files: "main.cpp"
}

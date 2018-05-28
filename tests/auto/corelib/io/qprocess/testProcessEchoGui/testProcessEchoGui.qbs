import qbs

QtAutotestHelperApp {
    condition: base && qbs.targetOS.contains("windows")
    consoleApplication: false

    Depends { name: "Qt.gui" }

    cpp.dynamicLibraries: base.concat(["user32"])
    files: "main_win.cpp"
}

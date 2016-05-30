import qbs

QProcessTestHelper {
    condition: qbs.targetOS.contains("windows")
    consoleApplication: false
    hasStandardMainFile: false
    cpp.dynamicLibraries: base.concat(["user32"])
    cpp.defines: base.concat(["QT_DISABLE_DEPRECATED_BEFORE=0"])
    files: ["main_win.cpp"]
}

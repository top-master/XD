import qbs

QtAutotestHelperApp {
    Depends { name: "Qt.core" }

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: base.concat(["user32"])
    }

    Group {
        name: "Windows sources"
        condition: qbs.targetOS.contains("windows")
        files: ["main_win.cpp"]
    }
    Group {
        name: "Unix sources"
        condition: qbs.targetOS.contains("unix")
        files: ["main_unix.cpp"]
    }
}

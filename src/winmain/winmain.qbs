import qbs

QtStaticLibrary {
    condition: qbs.targetOS.contains("windows")
    name: "winmain"
    isInternal: false
    targetName: "qtmain"

    Depends { name: "Qt.core" }
    cpp.defines: ["QT_NO_FOREACH"]

    Properties {
        condition: qbs.toolchain.contains("mingw")
        commonCppDefines: outer.concat("QT_NEEDS_QMAIN")
    }

    Group {
        name: "winrt"
        condition: product.targetsUWP
        files: ["qtmain_winrt.cpp"]
    }
    Group {
        name: "win32"
        condition: !product.targetsUWP
        files: ["qtmain_win.cpp"]
    }
    Export {
        Properties {
            condition: !product.targetsUWP
            cpp.dynamicLibraries: ["shell32"]
        }
    }
}

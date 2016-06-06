import qbs

QtProduct {
    name: "qtmain"
    targetName: name + (qbs.enableDebugCode && qbs.targetOS.contains("windows") ? 'd' : '') // TODO: infix
    condition: qbs.targetOS.contains("windows")
    type: ["staticlibrary"]
    version: project.version
    destinationDirectory: project.libDirectory

    Depends { name: "Qt.core" }

    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: base.concat(["-Z7"])
    }
    Properties {
        condition: qbs.toolchain.contains("mingw")
        cpp.defines: base.concat(["QT_NEEDS_QMAIN"])
    }
    Properties {
        condition: !qbs.targetOS.contains("winrt")
        cpp.dynamicLibraries: base.concat(["shell32"])
    }

    Export {
        Depends { name: "cpp" }
        Properties {
            condition: qbs.toolchain.contains("mingw")
            cpp.defines: base.concat(["QT_NEEDS_QMAIN"])
        }
        Properties {
            condition: !qbs.targetOS.contains("winrt")
            cpp.dynamicLibraries: base.concat(["shell32"])
        }
    }

    Group {
        name: "winrt sources"
        condition: qbs.targetOS.contains("winrt")
        files: [
            "qtmain_winrt.cpp",
        ]
    }

    Group {
        name: "non-winrt sources"
        condition: !qbs.targetOS.contains("winrt")
        files: [
            "qtmain_win.cpp",
        ]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "lib"
    }
}

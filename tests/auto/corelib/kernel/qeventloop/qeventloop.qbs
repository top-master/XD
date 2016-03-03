import qbs

QtAutotest {
    name: "tst_qeventloop"
    condition: project.network
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network" }
    files: ["tst_qeventloop.cpp"]

    Properties {
        condition: qbs.targetOS.contains("windows")
            && !qbs.targetOS.contains("wince")
            && !qbs.targetOS.contains("winrt")
        cpp.dynamicLibraries: base.concat("user32")
    }

    Properties {
        condition: project.glib
        cpp.defines: base.concat("HAVE_GLIB")
    }
}

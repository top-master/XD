import qbs

QtAutotest {
    name: "tst_qsettings"
    condition: Qt.gui.present
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui"; required: false }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.dynamicLibraries: base.concat(["advapi32.lib"])
    }

    cpp.defines: base.concat(["QT_DISABLE_DEPRECATED_BEFORE=0"])
    files: [
        "qsettings.qrc",
        "tst_qsettings.cpp",
    ]
}

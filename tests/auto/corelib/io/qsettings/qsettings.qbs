import qbs

QtAutotest {
    name: "tst_qsettings"
    condition: Qt.global.privateConfig.gui

    Depends { name: "Qt.global" }
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui" }

    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.dynamicLibraries: base.concat(["advapi32.lib"])
    }

    cpp.defines: base.concat(["QT_DISABLE_DEPRECATED_BEFORE=0"])
    cpp.includePaths: base.concat("../../kernel/qmetatype")
    files: [
        "qsettings.qrc",
        "tst_qsettings.cpp",
    ]
}

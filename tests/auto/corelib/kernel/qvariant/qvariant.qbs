import qbs

QtAutotest {
    name: "tst_qvariant"
    supportsUiKit: true
    Depends { name: "Qt.core_private" }
    cpp.includePaths: base.concat("../../../other/qvariant_common")
    cpp.defines: {
        var list = base.concat("QT_DISABLE_DEPRECATED_BEFORE=0");
        if (!Qt.core_private.config.doubleconversion
                && !Qt.core_private.config.system_doubleConversion) {
            list.push("QT_NO_DOUBLECONVERSION");
        }
        return list;
    }
    cpp.cxxLanguageVersion: Qt.global.config.c__1z
                            ? "c++17" : Qt.global.config.c__14
                            ? "c++14" : original
    files: [
        "tst_qvariant.cpp",
        "qvariant.qrc"
    ]
}

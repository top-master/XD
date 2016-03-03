import qbs

QtAutotest {
    name: "tst_qvariant"
    Depends { name: "Qt.core-private" }
    cpp.includePaths: base.concat("../../../other/qvariant_common")
    cpp.defines: {
        var list = base.concat("QT_DISABLE_DEPRECATED_BEFORE=0");
        if (!project.doubleConversion && !project.system_doubleConversion)
            list.push("QT_NO_DOUBLECONVERSION");
        return list;
    }
    Properties {
        condition: project.config.contains("c++11")
        cpp.cxxLanguageVersion: "c++11"
    }
    files: [
        "tst_qvariant.cpp",
        "qvariant.qrc"
    ]
}

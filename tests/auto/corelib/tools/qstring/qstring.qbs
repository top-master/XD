import qbs

QtAutotest {
    name: "tst_qstring"

    Properties {
        condition: qbs.targetOS.contains("darwin")
        cpp.frameworks: base.concat(["Foundation"])
    }

    cpp.defines: {
        var defines = base.concat(["QT_NO_CAST_TO_ASCII", "QT_DISABLE_DEPRECATED_BEFORE=0"]);
        if (project.icu)
            defines.push("QT_USE_ICU");
        if (!project.doubleConversion && !project.system_doubleConversion)
            defines.push("QT_NO_DOUBLECONVERSION");
        return defines;
    }

    files: [
        "tst_qstring.cpp",
    ]

    Group {
        name: "mac sources"
        condition: qbs.targetOS.contains("darwin")
        files: [
            "tst_qstring_mac.mm",
        ]
    }
}

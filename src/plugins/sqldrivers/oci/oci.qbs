import qbs

QtSqlPlugin {
    condition: project.sqlPlugins.contains("oci")
    name: "qsqloci"
    files: [
        "main.cpp",
        "oci.json",
    ]

    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/oci/"
        files: [
            "qsql_oci.cpp",
            "qsql_oci_p.h",
        ]
    }

    Properties {
        condition: qbs.targetOS.contains("darwin")
        cpp.linkerFlags: base.concat("-flat_namespace", "-U", "_environ")
    }

    cpp.dynamicLibraries: {
        var libs = base;
        var libsString = libs.toString();
        if (qbs.targetOS.contains("unix")) {
            if (libsString.search(/clnts/) != -1)
                libs.push("clntsh");
        } else {
            libs.push("oci");
        }
        return libs;
    }
}

// PLUGIN_CLASS_NAME = QOCIDriverPlugin

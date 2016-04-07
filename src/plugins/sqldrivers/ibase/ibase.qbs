import qbs

QtSqlPlugin {
    condition: project.sqlPlugins.contains("ibase")
    name: "qsqlibase"
    files: [
        "main.cpp",
        "ibase.json",
    ]
    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/ibase/"
        files: [
            "qsql_ibase.cpp",
            "qsql_ibase_p.h",
        ]
    }

    cpp.dynamicLibraries: {
        var libs = base;
        var libsString = libs.toString();
        if (libsString.search(/gds/) != -1)
            return libs;
        if (qbs.targetOS.contains("unix")) {
            if (!libsString.search(/fb/) != -1)
                libs.push("gds");
        } else if (!libsString.search(/fbclient/) != -1) {
            libs.push("gds32_ms");
        }
        return libs;
    }
}

// PLUGIN_CLASS_NAME = QIBaseDriverPlugin

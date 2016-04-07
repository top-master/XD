import qbs

QtSqlPlugin {
    condition: project.sqlPlugins.contains("sqlite2")
    name: "qsqlite2"
    files: [
        "smain.cpp",
        "sqlite2.json",
    ]
    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/sqlite2/"
        files: [
            "qsql_sqlite2.cpp",
            "qsql_sqlite2_p.h",
        ]
    }
    cpp.dynamicLibraries: {
        var libs = base || [];
        if (libs.toString().search(/sqlite/) == -1)
            libs.push("sqlite");
        return libs;
    }
}

// PLUGIN_CLASS_NAME = QSQLite2DriverPlugin

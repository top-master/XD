import qbs

QtSqlPlugin {
    condition: project.sqlPlugins.contains("db2")
    name: "qsqldb2"
    files: [
        "db2.json",
        "main.cpp",
    ]
    Group {
        name: "Sources from QtSql"
        prefix: "../../../sql/drivers/db2/"
        files: [
            "qsql_db2.cpp",
            "qsql_db2_p.h",
        ]
    }

    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.dynamicLibraries: "db2"
    }
    cpp.dynamicLibraries: "db2cli"
}

// PLUGIN_CLASS_NAME = QDB2DriverPlugin

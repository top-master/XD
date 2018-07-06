import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_psql
    name: "qsqlpsql"
    pluginClassName: "QPSQLDriverPlugin"

    Depends { name: "Psql" }

    files: [
        "README",
        "main.cpp",
        "psql.json",
        "qsql_psql.cpp",
        "qsql_psql_p.h",
    ]
}

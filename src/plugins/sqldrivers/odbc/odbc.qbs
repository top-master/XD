import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_odbc
    name: "qsqlodbc"
    pluginClassName: "QODBCDriverPlugin"

    Depends { name: "Odbc" }

    files: [
        "README",
        "main.cpp",
        "odbc.json",
        "qsql_odbc.cpp",
        "qsql_odbc_p.h",
    ]

    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.defines: base.concat("UNICODE")
    }
}

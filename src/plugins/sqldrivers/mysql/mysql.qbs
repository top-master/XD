import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_mysql
    name: "qsqlmysql"
    pluginClassName: "QMYSQLDriverPlugin"

    files: [
        "README",
        "main.cpp",
        "mysql.json",
        "qsql_mysql.cpp",
        "qsql_mysql_p.h",
    ]
}

// QMAKE_USE += mysql

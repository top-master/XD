import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_mysql
    name: "qsqlmysql"
    pluginClassName: "QMYSQLDriverPlugin"

    Depends { name: "Mysql" }

    files: [
        "README",
        "main.cpp",
        "mysql.json",
        "qsql_mysql.cpp",
        "qsql_mysql_p.h",
    ]
}

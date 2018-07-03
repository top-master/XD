import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_db2
    name: "qsqldb2"
    pluginClassName: "QDB2DriverPlugin"

    Properties {
        condition: qbs.architecture === "x86_64"
        cpp.defines: base.concat("ODBC64")
    }

    files: [
        "README",
        "db2.json",
        "main.cpp",
        "qsql_db2.cpp",
        "qsql_db2_p.h",
    ]
}

// TODO (also in other sqldrivers): QMAKE_USE += db2

import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_tds
    name: "qsqltds"
    pluginClassName: "QTDSDriverPlugin"

    files: [
        "main.cpp",
        "qsql_tds.cpp",
        "qsql_tds_p.h",
        "tds.json",
    ]
}

// QMAKE_USE += tds

import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_tds
    name: "qsqltds"
    pluginClassName: "QTDSDriverPlugin"

    Depends { name: "Tds" }

    files: [
        "main.cpp",
        "qsql_tds.cpp",
        "qsql_tds_p.h",
        "tds.json",
    ]
}

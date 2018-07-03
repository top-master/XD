import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_sqlite
    name: "qsqlite"
    pluginClassName: "QSQLiteDriverPlugin"

    Depends { name: "qt_libsqlite" }

    files: [
        "README",
        "smain.cpp",
        "qsql_sqlite.cpp",
        "qsql_sqlite_p.h",
        "sqlite.json",
    ]
}

// QT_FOR_CONFIG += sqldrivers-private

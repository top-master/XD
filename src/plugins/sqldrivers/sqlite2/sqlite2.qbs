import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_sqlite2
    name: "qsqlite2"
    pluginClassName: "QSQLite2DriverPlugin"

    files: [
        "README",
        "smain.cpp",
        "qsql_sqlite2.cpp",
        "qsql_sqlite2_p.h",
        "sqlite2.json",
    ]
    cpp.dynamicLibraries: {
        var libs = base || [];
        if (libs.toString().search(/sqlite/) == -1)
            libs.push("sqlite");
        return libs;
    }
}

// QMAKE_USE += sqlite2

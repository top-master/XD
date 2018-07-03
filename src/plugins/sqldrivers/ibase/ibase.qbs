import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_ibase
    name: "qsqlibase"
    pluginClassName: "QIBaseDriverPlugin"

    files: [
        "main.cpp",
        "ibase.json",
        "qsql_ibase.cpp",
        "qsql_ibase_p.h",
    ]
}

/*
# FIXME: ignores libfb (unix)/fbclient (win32) - but that's for the test anyway
QMAKE_USE += ibase
*/

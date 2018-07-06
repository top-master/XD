import qbs
import QtSqldriversPrivateConfig

QtSqlPlugin {
    condition: base && QtSqldriversPrivateConfig.sql_oci
    name: "qsqloci"
    pluginClassName: "QOCIDriverPlugin"

    Depends { name: "Oci" }

    files: [
        "README",
        "main.cpp",
        "oci.json",
        "qsql_oci.cpp",
        "qsql_oci_p.h",
    ]

    Properties {
        condition: qbs.targetOS.contains("darwin")
        cpp.linkerFlags: base.concat("-flat_namespace", "-U", "_environ")
    }
}

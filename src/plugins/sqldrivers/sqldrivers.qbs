import qbs

Project {
    name: "sqldrivers"
    qbsSearchPaths: [".", project.qtbaseShadowDir + "/src/plugins/sqldrivers/qbs"]
    references: [
        "db2",
        "ibase",
        "mysql",
        "oci",
        "odbc",
        "psql",
        "sqlite",
        "sqlite2",
        "tds",
    ]
}

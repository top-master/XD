import qbs

Project {
    name: "sqldrivers"
    qbsSearchPaths: ["qbs"]
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

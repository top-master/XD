import qbs

Project {
    name: "sql"
    references: [
        "books",
        "cachedtable",
        "drilldown",
        "masterdetail",
        "querymodel",
        "relationaltablemodel",
        "sqlbrowser",
        "sqlwidgetmapper",
        "tablemodel",
    ]
}

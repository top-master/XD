import qbs

QtAutotestProject {
    name: "corelib_itemmodel_tests"
    references: [
        "qabstractitemmodel",
        "qabstractproxymodel",
        "qidentityproxymodel",
        "qitemmodel",
        "qitemselectionmodel",
        "qsortfilterproxymodel",
        "qsortfilterproxymodel_recursive",
        "qstringlistmodel",
    ]
}

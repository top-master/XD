import qbs

QtAutotestProject {
    name: "corelib_itemmodel_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "qabstractitemmodel",
        "qabstractproxymodel",
        "qidentityproxymodel",
        "qitemmodel",
        "qitemselectionmodel",
        "qsortfilterproxymodel",
        "qstringlistmodel",
    ]
}

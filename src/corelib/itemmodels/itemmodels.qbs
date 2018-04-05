import qbs

Group {
    condition: QtCoreConfig.itemmodel
    name: "item models"
    prefix: path + '/'
    files: [
        "qabstractitemmodel.cpp",
        "qabstractitemmodel.h",
        "qabstractitemmodel_p.h",
        "qidentityproxymodel.cpp",
        "qidentityproxymodel.h",
        "qitemselectionmodel.cpp",
        "qitemselectionmodel.h",
        "qitemselectionmodel_p.h",
        "qsortfilterproxymodel.cpp",
        "qsortfilterproxymodel.h",
        "qstringlistmodel.cpp",
        "qstringlistmodel.h",
    ]
}

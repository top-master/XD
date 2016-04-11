import qbs

QtExample {
    name: "simpletreemodel"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "simpletreemodel.qrc",
        "treeitem.cpp",
        "treeitem.h",
        "treemodel.cpp",
        "treemodel.h",
    ]
}

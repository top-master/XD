import qbs

QtExample {
    name: "editabletreemodel"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "editabletreemodel.qrc",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "mainwindow.ui",
        "treeitem.cpp",
        "treeitem.h",
        "treemodel.cpp",
        "treemodel.h",
    ]
}

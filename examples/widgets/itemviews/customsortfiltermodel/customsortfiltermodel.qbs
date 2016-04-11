import qbs

QtExample {
    name: "customsortfiltermodel"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "customsortfiltermodel.qrc",
        "filterwidget.cpp",
        "filterwidget.h",
        "main.cpp",
        "mysortfilterproxymodel.cpp",
        "mysortfilterproxymodel.h",
        "window.cpp",
        "window.h",
    ]
}

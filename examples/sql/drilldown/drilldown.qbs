import qbs

QtExample {
    name: "drilldown"
    condition: Qt.sql.present && Qt.widgets.present

    Depends { name: "Qt.sql"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "../connection.h",
        "drilldown.qrc",
        "imageitem.cpp",
        "imageitem.h",
        "informationwindow.cpp",
        "informationwindow.h",
        "main.cpp",
        "view.cpp",
        "view.h",
    ]
}

import qbs

QtExample {
    name: "querymodel"
    condition: Qt.sql.present && Qt.widgets.present

    Depends { name: "Qt.sql"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "../connection.h",
        "customsqlmodel.cpp",
        "customsqlmodel.h",
        "editablesqlmodel.cpp",
        "editablesqlmodel.h",
        "main.cpp",
    ]
}

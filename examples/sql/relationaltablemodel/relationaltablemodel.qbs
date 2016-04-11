import qbs

QtExample {
    name: "relationaltablemodel"
    condition: Qt.sql.present && Qt.widgets.present

    Depends { name: "Qt.sql"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "../connection.h",
        "relationaltablemodel.cpp",
    ]
}

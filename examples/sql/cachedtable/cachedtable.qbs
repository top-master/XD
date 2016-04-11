import qbs

QtExample {
    name: "cachedtable"
    condition: Qt.sql.present && Qt.widgets.present

    Depends { name: "Qt.sql"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "../connection.h",
        "main.cpp",
        "tableeditor.cpp",
        "tableeditor.h",
    ]
}

import qbs

QtExample {
    name: "sqlwidgetmapper"
    condition: Qt.sql.present && Qt.widgets.present

    Depends { name: "Qt.sql"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}

import qbs

QtExample {
    name: "frozencolumn"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "freezetablewidget.cpp",
        "freezetablewidget.h",
        "grades.qrc",
        "main.cpp",
    ]
}

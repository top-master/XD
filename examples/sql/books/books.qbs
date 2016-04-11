import qbs

QtExample {
    name: "books"
    condition: Qt.sql.present && Qt.widgets.present

    Depends { name: "Qt.sql"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    cpp.includePaths: base.concat(".")

    files: [
        "bookdelegate.cpp",
        "bookdelegate.h",
        "books.qrc",
        "bookwindow.cpp",
        "bookwindow.h",
        "bookwindow.ui",
        "initdb.h",
        "main.cpp",
    ]
}

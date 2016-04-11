import qbs

QtExample {
    name: "spreadsheet"
    condition: Qt.widgets.present

    Depends { name: "Qt.printsupport"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "printview.cpp",
        "printview.h",
        "spreadsheet.cpp",
        "spreadsheet.h",
        "spreadsheet.qrc",
        "spreadsheetdelegate.cpp",
        "spreadsheetdelegate.h",
        "spreadsheetitem.cpp",
        "spreadsheetitem.h",
    ]
}

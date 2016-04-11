import qbs

QtExample {
    name: "masterdetail"

    condition: Qt.sql.present && Qt.widgets.present && Qt.xml.present

    Depends { name: "Qt.sql"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    Depends { name: "Qt.xml"; required: false }

    files: [
        "albumdetails.xml",
        "database.h",
        "dialog.cpp",
        "dialog.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "masterdetail.qrc",
    ]
}

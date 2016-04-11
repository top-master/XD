import qbs

QtExample {
    name: "addressbook"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "adddialog.cpp",
        "adddialog.h",
        "addresswidget.cpp",
        "addresswidget.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "newaddresstab.cpp",
        "newaddresstab.h",
        "tablemodel.cpp",
        "tablemodel.h",
    ]
}

import qbs

QtExample {
    name: "orderform"
    condition: Qt.widgets.present

    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.printsupport"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "detailsdialog.cpp",
        "detailsdialog.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}

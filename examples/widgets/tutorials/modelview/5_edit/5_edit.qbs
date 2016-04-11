import qbs

QtExample {
    name: "mv_edit"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "mymodel.cpp",
        "mymodel.h",
    ]
}

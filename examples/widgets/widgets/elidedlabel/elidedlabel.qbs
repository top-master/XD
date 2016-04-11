import qbs

QtExample {
    name: "elidedlabel"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "elidedlabel.cpp",
        "elidedlabel.h",
        "main.cpp",
        "testwidget.cpp",
        "testwidget.h",
    ]
}

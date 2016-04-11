import qbs

QtExample {
    name: "stylesheet"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "mainwindow.ui",
        "stylesheet.qrc",
        "stylesheeteditor.cpp",
        "stylesheeteditor.h",
        "stylesheeteditor.ui",
    ]
}

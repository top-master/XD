import qbs

QtExample {
    name: "simpledommodel"
    condition: Qt.widgets.present && Qt.xml.present

    Depends { name: "Qt.widgets"; required: false }
    Depends { name: "Qt.xml"; required: false }

    files: [
        "domitem.cpp",
        "domitem.h",
        "dommodel.cpp",
        "dommodel.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}

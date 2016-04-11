import qbs

QtExample {
    name: "diagramscene"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "arrow.cpp",
        "arrow.h",
        "diagramitem.cpp",
        "diagramitem.h",
        "diagramscene.cpp",
        "diagramscene.h",
        "diagramscene.qrc",
        "diagramtextitem.cpp",
        "diagramtextitem.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}

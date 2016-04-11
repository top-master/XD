import qbs

QtExample {
    name: "basicgraphicslayouts"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "basicgraphicslayouts.qrc",
        "layoutitem.cpp",
        "layoutitem.h",
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}

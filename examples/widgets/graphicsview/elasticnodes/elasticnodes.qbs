import qbs

QtExample {
    name: "elasticnodes"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "edge.cpp",
        "edge.h",
        "graphwidget.cpp",
        "graphwidget.h",
        "main.cpp",
        "node.cpp",
        "node.h",
    ]
}

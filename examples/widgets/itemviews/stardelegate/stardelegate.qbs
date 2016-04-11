import qbs

QtExample {
    name: "stardelegate"
    condition: Qt.widgets.present

    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "stardelegate.cpp",
        "stardelegate.h",
        "stareditor.cpp",
        "stareditor.h",
        "starrating.cpp",
        "starrating.h",
    ]
}

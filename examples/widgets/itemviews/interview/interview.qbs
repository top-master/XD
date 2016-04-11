import qbs

QtExample {
    name: "interview"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "interview.qrc",
        "main.cpp",
        "model.cpp",
        "model.h",
    ]
}

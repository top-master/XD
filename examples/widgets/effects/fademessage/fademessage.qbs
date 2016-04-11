import qbs

QtExample {
    name: "fademessage"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "fademessage.cpp",
        "fademessage.h",
        "fademessage.qrc",
        "main.cpp",
    ]
}

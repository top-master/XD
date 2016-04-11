import qbs

QtExample {
    name: "collidingmice"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mice.qrc",
        "mouse.cpp",
        "mouse.h",
    ]
}

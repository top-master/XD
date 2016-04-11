import qbs

QtExample {
    name: "rogue"
    condition: Qt.widgets.present

    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "movementtransition.h",
        "window.cpp",
        "window.h",
    ]
}

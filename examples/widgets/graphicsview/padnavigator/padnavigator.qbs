import qbs

QtExample {
    name: "padnavigator"
    condition: Qt.widgets.present
    consoleApplication: true

    Depends { name: "Qt.opengl"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "flippablepad.cpp",
        "flippablepad.h",
        "form.ui",
        "main.cpp",
        "padnavigator.cpp",
        "padnavigator.h",
        "padnavigator.qrc",
        "roundrectitem.cpp",
        "roundrectitem.h",
        "splashitem.cpp",
        "splashitem.h",
    ]
}

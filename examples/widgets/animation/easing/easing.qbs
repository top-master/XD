import qbs

QtExample {
    name: "easing"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "animation.h",
        "easing.qrc",
        "form.ui",
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}

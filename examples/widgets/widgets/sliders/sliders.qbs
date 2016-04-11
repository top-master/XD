import qbs

QtExample {
    name: "sliders"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "slidersgroup.cpp",
        "slidersgroup.h",
        "window.cpp",
        "window.h",
    ]
}

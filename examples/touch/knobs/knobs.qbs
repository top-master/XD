import qbs

QtExample {
    name: "knobs"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "knob.cpp",
        "knob.h",
        "main.cpp",
    ]
}

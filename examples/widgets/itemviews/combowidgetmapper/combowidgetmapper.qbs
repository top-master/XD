import qbs

QtExample {
    name: "combowidgetmapper"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}

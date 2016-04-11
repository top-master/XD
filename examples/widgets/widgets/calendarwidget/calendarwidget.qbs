import qbs

QtExample {
    name: "calendarwidget"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}

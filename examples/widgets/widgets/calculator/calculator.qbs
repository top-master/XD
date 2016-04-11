import qbs

QtExample {
    name: "calculator"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "button.cpp",
        "button.h",
        "calculator.cpp",
        "calculator.h",
        "main.cpp",
    ]
}

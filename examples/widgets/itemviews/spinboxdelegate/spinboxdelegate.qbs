import qbs

QtExample {
    name: "spinboxdelegate"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "delegate.cpp",
        "delegate.h",
        "main.cpp",
    ]
}

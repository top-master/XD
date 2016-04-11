import qbs

QtExample {
    name: "flickable"
    condition: Qt.widgets.present
    install: false
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "flickable.cpp",
        "flickable.h",
        "main.cpp",
    ]
}

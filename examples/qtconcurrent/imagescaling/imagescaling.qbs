import qbs

QtExample {
    name: "imagescaling"
    condition: Qt.concurrent.present && Qt.widgets.present

    Depends { name: "Qt.concurrent"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "imagescaling.cpp",
        "imagescaling.h",
        "main.cpp",
    ]
}

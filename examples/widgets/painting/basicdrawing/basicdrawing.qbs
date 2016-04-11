import qbs

QtExample {
    name: "basicdrawing"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "basicdrawing.qrc",
        "main.cpp",
        "renderarea.cpp",
        "renderarea.h",
        "window.cpp",
        "window.h",
    ]
}

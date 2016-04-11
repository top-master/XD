import qbs

QtExample {
    name: "embeddeddialogs"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "customproxy.cpp",
        "customproxy.h",
        "embeddeddialog.cpp",
        "embeddeddialog.h",
        "embeddeddialog.ui",
        "embeddeddialogs.qrc",
        "main.cpp",
    ]
}

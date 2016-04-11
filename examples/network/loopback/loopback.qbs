import qbs

QtExample {
    name: "loopback"
    condition: Qt.network.present && Qt.widgets.present

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "dialog.cpp",
        "dialog.h",
        "main.cpp",
    ]
}

import qbs

QtExample {
    name: "multicastsender"
    condition: Qt.network.present && Qt.widgets.present

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "sender.cpp",
        "sender.h",
    ]
}

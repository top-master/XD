import qbs

QtExample {
    name: "broadcastreceiver"
    condition: Qt.network.present && Qt.widgets.present

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "receiver.cpp",
        "receiver.h",
    ]
}

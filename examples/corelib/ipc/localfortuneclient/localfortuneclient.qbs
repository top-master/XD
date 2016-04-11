import qbs

QtExample {
    name: "localfortuneclient"
    condition: Qt.network.present && Qt.widgets.present
    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "client.cpp",
        "client.h",
        "main.cpp",
    ]
}

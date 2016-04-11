import qbs

QtExample {
    name: "localfortuneserver"
    condition: Qt.network.present && Qt.widgets.present
    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "main.cpp",
        "server.cpp",
        "server.h",
    ]
}

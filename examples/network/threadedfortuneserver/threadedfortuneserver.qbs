import qbs

QtExample {
    name: "threadedfortuneserver"
    condition: Qt.network.present && Qt.widgets.present

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "dialog.cpp",
        "dialog.h",
        "fortuneserver.cpp",
        "fortuneserver.h",
        "fortunethread.cpp",
        "fortunethread.h",
        "main.cpp",
    ]
}

import qbs

QtExample {
    name: "blockingfortuneclient"
    condition: Qt.network.present && Qt.widgets.present

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "blockingclient.cpp",
        "blockingclient.h",
        "fortunethread.cpp",
        "fortunethread.h",
        "main.cpp",
    ]
}

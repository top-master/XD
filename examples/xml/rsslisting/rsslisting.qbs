import qbs

QtExample {
    name: "rsslisting"
    condition: Qt.network.present && Qt.widgets.present && Qt.xml.present

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    Depends { name: "Qt.xml"; required: false }

    files: [
        "main.cpp",
        "rsslisting.cpp",
        "rsslisting.h",
    ]
}

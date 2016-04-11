import qbs

QtExample {
    name: "flightinfo"
    condition: Qt.network.present && Qt.widgets.present
    install: false
    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "flightinfo.cpp",
        "flightinfo.qrc",
        "form.ui",
    ]
}

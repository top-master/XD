import qbs

QtExample {
    name: "googlesuggest"
    condition: Qt.network.present && Qt.widgets.present

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "googlesuggest.cpp",
        "googlesuggest.h",
        "main.cpp",
        "searchbox.cpp",
        "searchbox.h",
    ]
}

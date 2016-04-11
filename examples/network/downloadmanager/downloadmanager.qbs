import qbs

QtExample {
    name: "downloadmanager"
    condition: Qt.network.present
    consoleApplication: true

    Depends { name: "Qt.network"; required: false }

    files: [
        "downloadmanager.cpp",
        "downloadmanager.h",
        "main.cpp",
        "textprogressbar.cpp",
        "textprogressbar.h",
    ]
}

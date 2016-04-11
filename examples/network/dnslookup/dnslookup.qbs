import qbs

QtExample {
    name: "dnslookup"
    condition: Qt.network.present
    consoleApplication: true

    Depends { name: "Qt.network"; required: false }

    files: [
        "dnslookup.cpp",
        "dnslookup.h",
    ]
}

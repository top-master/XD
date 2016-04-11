import qbs

QtExample {
    name: "fortuneserver"
    condition: Qt.network.present && Qt.widgets.present
               && !project.disabledFeatures.contains("bearermanagement")

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "server.cpp",
        "server.h",
    ]
}

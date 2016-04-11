import qbs

QtExample {
    name: "fortuneclient"
    condition: Qt.network.present && Qt.widgets.present
               && !project.disabledFeatures.contains("bearermanagement")

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "client.cpp",
        "client.h",
        "main.cpp",
    ]
}

import qbs

QtExample {
    name: "bearermonitor"
    condition: Qt.network.present && Qt.widgets.present
               && !project.disabledFeatures.contains("bearermanagement")
    consoleApplication: true

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "bearermonitor.cpp",
        "bearermonitor.h",
        "bearermonitor_240_320.ui",
        "bearermonitor_640_480.ui",
        "main.cpp",
        "sessionwidget.cpp",
        "sessionwidget.h",
        "sessionwidget.ui",
    ]

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: base.concat("ws2_32")
    }
}

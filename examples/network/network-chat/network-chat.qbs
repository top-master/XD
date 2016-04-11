import qbs

QtExample {
    name: "network-chat"
    condition: Qt.network.present && Qt.widgets.present
               && !project.disabledFeatures.contains("bearermanagement")
               && !qbs.targetOS.contains("vxworks")
               && !qbs.targetOS.contains("qbs")
               && !qbs.targetOS.contains("winrt")

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "chatdialog.cpp",
        "chatdialog.h",
        "chatdialog.ui",
        "client.cpp",
        "client.h",
        "connection.cpp",
        "connection.h",
        "main.cpp",
        "peermanager.cpp",
        "peermanager.h",
        "server.cpp",
        "server.h",
    ]
}

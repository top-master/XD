import qbs

QtExample {
    name: "securesocketclient"
    condition: Qt.network.present && Qt.widgets.present
               && (project.config.contains("openssl")
                   || project.config.contains("openssl-linked"))

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "certificateinfo.cpp",
        "certificateinfo.h",
        "certificateinfo.ui",
        "main.cpp",
        "securesocketclient.qrc",
        "sslclient.cpp",
        "sslclient.h",
        "sslclient.ui",
        "sslerrors.ui",
    ]
}

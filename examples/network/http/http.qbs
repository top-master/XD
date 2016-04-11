import qbs

QtExample {
    name: "http"
    condition: Qt.network.present && Qt.widgets.present

    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "authenticationdialog.ui",
        "httpwindow.cpp",
        "httpwindow.h",
        "main.cpp",
    ]
}

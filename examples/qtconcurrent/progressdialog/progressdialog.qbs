import qbs

QtExample {
    name: "progressdialog"
    condition: Qt.concurrent.present && Qt.widgets.present
    consoleApplication: true

    Depends { name: "Qt.concurrent"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}

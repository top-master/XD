import qbs

QtExample {
    name: "tutorial1"
    condition: Qt.testlib.present && Qt.widgets.present

    Depends { name: "Qt.testlib"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "testqstring.cpp",
    ]
}

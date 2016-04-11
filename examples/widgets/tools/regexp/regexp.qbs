import qbs

QtExample {
    name: "regexp"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "regexpdialog.cpp",
        "regexpdialog.h",
    ]
}

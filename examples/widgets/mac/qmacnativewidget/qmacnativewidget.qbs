import qbs

QtExample {
    name: "qmacnativewidget"
    condition: Qt.widgets.present && qbs.targetOS.contains("osx")

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.mm",
    ]

    cpp.frameworks: base.concat("AppKit")
}

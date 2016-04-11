import qbs

QtExample {
    name: "qcocoaviewcontainer"
    condition: Qt.widgets.present && qbs.targetOS.contains("osx")

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.mm",
    ]

    cpp.frameworks: base.concat("AppKit")
}

import qbs

QtExample {
    name: "trivialwizard"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_WIZARD")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "trivialwizard.cpp",
    ]
}

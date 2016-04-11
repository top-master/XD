import qbs

QtExample {
    name: "licensewizard"
    condition: Qt.printsupport.present && Qt.widgets.present
               && (!cpp.defines || !cpp.defines.contains("QT_NO_WIZARD")) // FIXME

    Depends { name: "Qt.printsupport"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "licensewizard.cpp",
        "licensewizard.h",
        "licensewizard.qrc",
        "main.cpp",
    ]
}

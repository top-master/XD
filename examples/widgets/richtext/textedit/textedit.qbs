import qbs

QtExample {
    name: "textedit"
    condition: Qt.widgets.present

    Depends { name: "Qt.printsupport"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "textedit.cpp",
        "textedit.h",
        "textedit.qdoc",
        "textedit.qrc",
    ]
}

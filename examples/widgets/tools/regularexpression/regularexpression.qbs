import qbs

QtExample {
    name: "regularexpression"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "regularexpressiondialog.cpp",
        "regularexpressiondialog.h",
        "regularexpression.qrc",
    ]
}

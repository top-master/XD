import qbs

QtExample {
    name: "mousebuttons"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "buttontester.cpp",
        "buttontester.h",
        "main.cpp",
    ]
}

import qbs

QtExample {
    name: "part3"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "addressbook.cpp",
        "addressbook.h",
        "main.cpp",
    ]
}

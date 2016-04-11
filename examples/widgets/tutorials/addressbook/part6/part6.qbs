import qbs

QtExample {
    name: "part6"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "addressbook.cpp",
        "addressbook.h",
        "finddialog.cpp",
        "finddialog.h",
        "main.cpp",
    ]
}

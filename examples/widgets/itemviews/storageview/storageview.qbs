import qbs

QtExample {
    name: "storageview"
    condition: Qt.widgets.present
    install: false

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "storagemodel.cpp",
        "storagemodel.h",
    ]
}

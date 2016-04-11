import qbs

QtExample {
    name: "blurpicker"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "blureffect.cpp",
        "blureffect.h",
        "blurpicker.cpp",
        "blurpicker.h",
        "blurpicker.qrc",
        "main.cpp",
    ]
}

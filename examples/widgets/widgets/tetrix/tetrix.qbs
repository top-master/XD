import qbs

QtExample {
    name: "tetrix"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "tetrixboard.cpp",
        "tetrixboard.h",
        "tetrixpiece.cpp",
        "tetrixpiece.h",
        "tetrixwindow.cpp",
        "tetrixwindow.h",
    ]
}

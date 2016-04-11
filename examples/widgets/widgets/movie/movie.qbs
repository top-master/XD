import qbs

QtExample {
    name: "movie"
    condition: Qt.widgets.present

    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "animation.gif",
        "main.cpp",
        "movieplayer.cpp",
        "movieplayer.h",
    ]
}

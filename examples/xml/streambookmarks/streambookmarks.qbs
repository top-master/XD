import qbs

QtExample {
    name: "streambookmarks"
    condition: Qt.widgets.present && Qt.xml.present

    Depends { name: "Qt.widgets"; required: false }
    Depends { name: "Qt.xml"; required: false }

    files: [
        "frank.xbel",
        "jennifer.xbel",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "xbelreader.cpp",
        "xbelreader.h",
        "xbelwriter.cpp",
        "xbelwriter.h",
    ]
}

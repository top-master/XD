import qbs

QtExample {
    name: "saxbookmarks"
    condition: Qt.widgets.present && Qt.xml.present

    Depends { name: "Qt.widgets"; required: false }
    Depends { name: "Qt.xml"; required: false }

    files: [
        "frank.xbel",
        "jennifer.xbel",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "xbelgenerator.cpp",
        "xbelgenerator.h",
        "xbelhandler.cpp",
        "xbelhandler.h",
    ]
}

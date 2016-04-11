import qbs

QtExample {
    name: "sub-attaq"
    condition: Qt.widgets.present

    Depends { name: "Qt.opengl"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "animationmanager.cpp",
        "animationmanager.h",
        "boat.cpp",
        "boat.h",
        "boat_p.h",
        "bomb.cpp",
        "bomb.h",
        "graphicsscene.cpp",
        "graphicsscene.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "pixmapitem.cpp",
        "pixmapitem.h",
        "progressitem.cpp",
        "progressitem.h",
        "qanimationstate.cpp",
        "qanimationstate.h",
        "states.cpp",
        "states.h",
        "subattaq.qrc",
        "submarine.cpp",
        "submarine.h",
        "submarine_p.h",
        "textinformationitem.cpp",
        "textinformationitem.h",
        "torpedo.cpp",
        "torpedo.h",
    ]
}

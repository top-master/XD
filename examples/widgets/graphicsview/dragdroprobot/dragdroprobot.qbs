import qbs

QtExample {
    name: "dragdroprobot"
    condition: Qt.widgets.present && (!cpp.defines || (!cpp.defines.contains("QT_NO_CURSOR")
               && !cpp.defines.contains("QT_NO_DRAGANDDROP"))) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "coloritem.cpp",
        "coloritem.h",
        "main.cpp",
        "robot.cpp",
        "robot.h",
        "robot.qrc",
    ]
}

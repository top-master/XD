import qbs

QtExample {
    name: "styles"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "norwegianwoodstyle.cpp",
        "norwegianwoodstyle.h",
        "styles.qrc",
        "widgetgallery.cpp",
        "widgetgallery.h",
    ]
}

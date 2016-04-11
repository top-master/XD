import qbs

QtExample {
    name: "plugandpaint"
    condition: Qt.widgets.present && !project.disabledFeatures.contains("library")

    Depends { name: "pnp_basictools"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "interfaces.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "paintarea.cpp",
        "paintarea.h",
        "plugindialog.cpp",
        "plugindialog.h",
    ]
}

import qbs

QtExample {
    name: "validators"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    cpp.includePaths: base.concat(".")

    files: [
        "ledwidget.cpp",
        "ledwidget.h",
        "localeselector.cpp",
        "localeselector.h",
        "main.cpp",
        "validators.qrc",
        "validators.ui",
    ]
}

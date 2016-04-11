import qbs

QtExample {
    name: "windowcontainer"
    condition: Qt.widgets.present && (project.opengl || project.opengles2)

    Depends { name: "Qt.widgets"; required: false }

    cpp.includePaths: base.concat("../../gui/openglwindow")

    files: [
        "windowcontainer.cpp",
    ]

    Group {
        name: "sources from openglwindow"
        prefix: "../../gui/openglwindow/"
        files: [
            "openglwindow.cpp",
            "openglwindow.h",
        ]
    }
}

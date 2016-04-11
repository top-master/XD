import qbs

 // qtHaveModule(opengl):!contains(QT_CONFIG, opengles.):!contains(QT_CONFIG,dynamicgl)

QtExample {
    name: "boxes"
    condition: Qt.opengl.present && Qt.widgets.present && !project.opengles2
            && !project.angle && !project.config.contains("dynamicgl")

    Depends { name: "Qt.opengl"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "3rdparty/fbm.c",
        "3rdparty/fbm.h",
        "boxes.qrc",
        "glbuffers.cpp",
        "glbuffers.h",
        "glextensions.cpp",
        "glextensions.h",
        "gltrianglemesh.h",
        "main.cpp",
        "qtbox.cpp",
        "qtbox.h",
        "roundedbox.cpp",
        "roundedbox.h",
        "scene.cpp",
        "scene.h",
        "trackball.cpp",
        "trackball.h",
    ]
}

import qbs

Project {
    references: [
        "deviceintegration/deviceintegration.qbs",
    ]

    QtModule {
        name: "Qt.egldeviceintegration"
        condition: project.egl && project.opengl
        simpleName: "egldeviceintegration"
        targetName: "Qt5EglDeviceIntegration"

        Depends { name: "egl" }
        Depends { name: "gl"; condition: !project.opengles2 }
        Depends { name: "glesv2"; condition: project.opengles2 }
        Depends { name: "Qt"; submodules: ["core", "core-private", "gui", "gui-private", "platformsupport-private"] }

        cpp.defines: [
            "QT_BUILD_EGL_DEVICE_LIB",
            "MESA_EGL_NO_X11_HEADERS",
        ].concat(base)

        Group {
            name: "sources"
            files: [
                "qeglfscontext.cpp",
                "qeglfscontext.h",
                "qeglfscursor.cpp",
                "qeglfscursor.h",
                "qeglfsdeviceintegration.cpp",
                "qeglfsdeviceintegration.h",
                "qeglfsglobal.h",
                "qeglfshooks.cpp",
                "qeglfshooks.h",
                "qeglfsintegration.cpp",
                "qeglfsintegration.h",
                "qeglfsoffscreenwindow.cpp",
                "qeglfsoffscreenwindow.h",
                "qeglfsscreen.cpp",
                "qeglfsscreen.h",
                "qeglfswindow.cpp",
                "qeglfswindow.h",
            ]
        }

        Group {
            name: "sources (cursor)"
            condition: !project.disabledFeatures.contains("cursor")
            files: [
                "cursor-atlas.png",
                "cursor.json",
                "cursor.qrc",
            ]
        }
    }

    QtPlugin {
        name: "qeglfs"
        condition: project.egl && project.opengl && qbs.targetOS.contains("linux")
        category: "platforms"

        Depends { name: "egl" }
        Depends { name: "gl"; condition: project.opengl && !project.opengles2 }
        Depends { name: "glesv2"; condition: project.opengles2 }
        Depends { name: "Qt"; submodules: ["core", "gui", "gui-private", "egldeviceintegration"] }

        cpp.defines: [
            "MESA_EGL_NO_X11_HEADERS",
        ].concat(base)

        Group {
            name: "sources"
            files: [
                "qeglfsmain.cpp",
                "eglfs.json",
            ]
        }
    }
}

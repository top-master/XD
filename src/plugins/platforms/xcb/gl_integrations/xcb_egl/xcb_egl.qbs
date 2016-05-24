import qbs

QtPlugin {
    condition: project.xcb && project.egl && project.opengl
    name: "qxcb-egl-integration"
    category: "xcbglintegrations"

    cpp.defines: [
        "MESA_EGL_NO_X11_HEADERS",
    ].concat(base)

    cpp.includePaths: [
        path + "/../",
        path + "/../..",
    ].concat(base)

    Depends { name: "egl" }
    Depends { name: "gl"; condition: !project.opengles2 }
    Depends { name: "glesv2"; condition: project.opengles2 }
    Depends { name: "Qt"; submodules: ["core", "core-private", "gui", "gui-private", "platformsupport-private", "xcbqpa"] }

    Group {
        name: "headers"
        files: [
            "qxcbeglcontext.h",
            "qxcbeglinclude.h",
            "qxcbeglintegration.cpp",
            "qxcbeglintegration.h",
            "qxcbeglmain.cpp",
            "qxcbeglnativeinterfacehandler.cpp",
            "qxcbeglnativeinterfacehandler.h",
            "qxcbeglwindow.cpp",
            "qxcbeglwindow.h",
            "xcb_egl.json",
        ]
    }
}

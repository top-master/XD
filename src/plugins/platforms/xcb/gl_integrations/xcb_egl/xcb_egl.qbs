import qbs

QtGlIntegrationPlugin {
    condition: project.xcb && project.egl && project.opengl
    name: "qxcb-egl-integration"

    cpp.defines: [
        "MESA_EGL_NO_X11_HEADERS",
    ].concat(base)

    Depends { name: "egl" }
    Depends { name: "gl"; condition: !project.opengles2 }
    Depends { name: "glesv2"; condition: project.opengles2 }

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

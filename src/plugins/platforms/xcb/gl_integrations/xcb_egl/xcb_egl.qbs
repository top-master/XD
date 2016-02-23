import qbs
import "../QtXcbGlIntegrationPlugin.qbs" as QtXcbGlIntegrationPlugin

QtXcbGlIntegrationPlugin {
    condition: project.xcb && project.egl

    cpp.defines: [
        "MESA_EGL_NO_X11_HEADERS",
    ].concat(base)

    cpp.includePaths: [
        "../",
    ].concat(base)

    Depends { name: "egl" }
    Depends { name: "gl"; condition: project.opengl }

    Group {
        name: "headers"
        prefix: basePath + "/gl_integrations/xcb_egl/"
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

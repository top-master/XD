import qbs

QtGlIntegrationPlugin {
    name: "qxcb-glx-integration"
    condition: project.xcb_xlib && project.opengl && !project.opengles2

    Depends { name: "qglxconvenience" }

    cpp.defines: {
        var defines = base.concat(["XCB_USE_GLX"]);
        if (project.xcb_glx)
            defines.push("XCB_HAS_XCB_GLX");
        return defines;
    }
    cpp.dynamicLibraries: {
        var libs = base;
        if (project.xcb_glx)
            libs.push("xcb-glx");
        if (!qbs.targetOS.contains("bsd"))
            libs.push("dl");
        return libs;
    }

    files: [
        "qxcbglxintegration.cpp",
        "qglxintegration.h",
        "qglxintegration.cpp",
        "qxcbglxintegration.h",
        "qxcbglxmain.cpp",
        "qxcbglxnativeinterfacehandler.cpp",
        "qxcbglxnativeinterfacehandler.h",
        "qxcbglxwindow.cpp",
        "qxcbglxwindow.h",
    ]
}

// PLUGIN_CLASS_NAME = QXcbGlxIntegrationPlugin

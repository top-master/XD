import qbs

QtPlugin {
    name: "qxcb-glx-integration"
    condition: project.xcb_xlib && project.opengl && !project.opengles2
    category: "xcbglintegrations"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "Qt.xcbqpa" }
    Depends { name: "qglxconvenience" }

    cpp.defines: {
        var defines = base.concat(["XCB_USE_GLX", "XCB_USE_XLIB"]);
        if (project.xcb_glx)
            defines.push("XCB_HAS_XCB_GLX");
        return defines;
    }
    cpp.dynamicLibraries: {
        var libs = base.concat(["xcb"]);
        if (project.xcb_glx)
            libs.push("xcb-glx");
        if (!qbs.targetOS.contains("bsd"))
            libs.push("dl");
        return libs;
    }
    cpp.includePaths: base.concat(["../", "../.."])

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
// include(../gl_integrations_plugin_base.pri)

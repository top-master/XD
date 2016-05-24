import qbs

Project {
    references: [
        "gl_integrations/gl_integrations.qbs",
    ]

    PkgConfigDependency {
        name: "xinput2"
        packageName: "xi"

        Export {
            Depends { name: "cpp" }
            cpp.defines: {
                var defines = [];
                var versionParts = (product.version || "").split('.');
                if (versionParts[2])
                    defines.push("LIBXI_PATCH=" + versionParts[2]);
                if (versionParts[1])
                    defines.push("LIBXI_MINOR=" + versionParts[1]);
                if (versionParts[0])
                    defines.push("LIBXI_MAJOR=" + versionParts[0]);
                return defines;
            }
        }
    }

    QtModule {
        name: "Qt.xcbqpa"
        simpleName: "xcbqpa"
        targetName: "Qt5XcbQpa"
        condition: project.xcb

        Depends { name: "freetype2" }
        Depends { name: "glib"; condition: project.glib }
        Depends { name: "xkbcommon-x11" }
        Depends { name: "xcb-icccm" }
        Depends { name: "xcb-image" }
        Depends { name: "xcb-keysyms" }
        Depends { name: "xcb-xkb" }
        Depends { name: "xcb-randr" }
        Depends { name: "xcb-xfixes" }
        Depends { name: "xcb-xinerama" }
        Depends { name: "xcb-sync" }
        Depends { name: "xinput2"; condition: project.xinput2 }
        Depends {
            name: "Qt"
            submodules: [
                "core", "core-private", "dbus", "dbus-private",
                "gui", "gui-private", "platformsupport-private"
            ]
        }

        cpp.defines: {
            var defines = base.concat(["QT_BUILD_XCB_PLUGIN",
                                       "QT_NO_ACCESSIBILITY_ATSPI_BRIDGE" /* FIXME */]);
            if (project.xcb_xlib)
                defines.push("XCB_USE_XLIB");
            if (project.xinput2)
                defines.push("XCB_USE_XINPUT2");
            return defines;
        }

        cpp.includePaths: [
            path,
            "gl_integrations",
        ].concat(base)

        cpp.dynamicLibraries: {
            var libs = base;
            if (project.xcb_xlib) {
                libs.push("X11", "X11-xcb");
            }
            return libs;
        }

        Group {
            name: "xinput"
            condition: project.xcb_xlib && project.xinput2
            files: ["qxcbconnection_xi2.cpp"]
        }

        Group {
            name: "sources"
            files: [
                "qxcbbackingstore.cpp",
                "qxcbbackingstore.h",
                "qxcbclipboard.cpp",
                "qxcbclipboard.h",
                "qxcbconnection.cpp",
                "qxcbconnection.h",
                "qxcbcursor.cpp",
                "qxcbcursor.h",
                "qxcbdrag.cpp",
                "qxcbdrag.h",
                "qxcbimage.cpp",
                "qxcbimage.h",
                "qxcbintegration.cpp",
                "qxcbintegration.h",
                "qxcbkeyboard.cpp",
                "qxcbkeyboard.h",
                "qxcbmime.cpp",
                "qxcbmime.h",
                "qxcbnativeinterface.cpp",
                "qxcbnativeinterface.h",
                "qxcbobject.h",
                "qxcbscreen.cpp",
                "qxcbscreen.h",
                "qxcbsystemtraytracker.cpp",
                "qxcbsystemtraytracker.h",
                "qxcbwindow.cpp",
                "qxcbwindow.h",
                "qxcbwmsupport.cpp",
                "qxcbwmsupport.h",
                "qxcbxsettings.cpp",
                "qxcbxsettings.h",
                "gl_integrations/qxcbglintegration.cpp",
                "gl_integrations/qxcbglintegration.h",
                "gl_integrations/qxcbglintegrationfactory.cpp",
                "gl_integrations/qxcbglintegrationfactory.h",
                "gl_integrations/qxcbglintegrationplugin.h",
                "gl_integrations/qxcbnativeinterfacehandler.cpp",
                "gl_integrations/qxcbnativeinterfacehandler.h",
            ]
        }
    }

    QtPlugin {
        name: "qxcb"
        condition: project.xcb && qbs.targetOS.contains("linux")
        category: "platforms"

        Depends { name: "Qt"; submodules: ["core", "gui", "gui-private", "xcbqpa"] }

        Group {
            name: "sources"
            files: "qxcbmain.cpp"
        }
    }
}

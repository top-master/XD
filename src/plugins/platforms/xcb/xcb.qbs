import qbs

Project {
    references: [
        "gl_integrations/xcb_egl/xcb_egl.qbs",
    ]

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
        Depends {
            name: "Qt"
            submodules: [
                "core", "core-private", "dbus", "dbus-private",
                "gui", "gui-private", "platformsupport-private"
            ]
        }

        cpp.defines: [
            "QT_BUILD_XCB_PLUGIN",
            "QT_NO_ACCESSIBILITY_ATSPI_BRIDGE", // ###fixme
        ].concat(base)

        cpp.includePaths: [
            path,
            "gl_integrations",
        ].concat(base)

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

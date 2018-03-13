import qbs

/*
 The device integration plugin base class has to live in a shared library,
 placing it into a static lib like platformsupport is not sufficient since we
 have to keep the QObject magic like qobject_cast working.
 Hence this private-only module.
 By having _p headers, it also enables developing out-of-tree integration plugins.
*/

QtModule {
    Depends { name: "QtGuiConfig" }
    Depends { name: "QtGuiPrivateConfig" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.devicediscovery_support-private" }
    Depends { name: "Qt.egl_support-private" }
    Depends { name: "Qt.eventdispatcher_support-private" }
    Depends { name: "Qt.fontdatabase_support-private" }
    Depends { name: "Qt.fb_support-private" }
    Depends { name: "Qt.service_support-private" }
    Depends { name: "Qt.theme_support-private" }
    Depends { name: "Qt.input_support-private"; required: false }
    Depends { name: "Qt.platformcompositor_support-private"; required: false }
    Depends { name: "Egl" }
    Depends { name: "QtPlatformHeaders" }
    Export {
        Depends { name: "Qt.egl_support-private" }
    }

    cpp.defines: {
        var result = base.concat("QT_BUILD_EGL_DEVICE_LIB");

        // Avoid X11 header collision, use generic EGL native types
        result.push("QT_EGL_NO_X11");

        return result;
    }

    Group {
        prefix: "api/"
        files: [
            "qeglfsdeviceintegration.cpp",
            "qeglfsdeviceintegration_p.h",
            "qeglfsglobal_p.h",
            "qeglfshooks.cpp",
            "qeglfshooks_p.h",
            "qeglfsintegration.cpp",
            "qeglfsintegration_p.h",
            "qeglfsoffscreenwindow.cpp",
            "qeglfsoffscreenwindow_p.h",
            "qeglfsscreen.cpp",
            "qeglfsscreen_p.h",
            "qeglfswindow.cpp",
            "qeglfswindow_p.h",
        ]

        Group {
            condition: QtGuiConfig.opengl
            files: [
                "qeglfscontext.cpp",
                "qeglfscontext_p.h",
                "qeglfscursor.cpp",
                "qeglfscursor_p.h",
            ]
        }
    }
    Group {
        condition: QtGuiConfig.cursor
        files: [
            "cursor.qrc",
        ]
    }
}

/*
# TODO: mkspecs fill this variable to pull in source files.
!isEmpty(EGLFS_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$EGLFS_PLATFORM_HOOKS_HEADERS
    SOURCES += $$EGLFS_PLATFORM_HOOKS_SOURCES
    LIBS    += $$EGLFS_PLATFORM_HOOKS_LIBS
    DEFINES += EGLFS_PLATFORM_HOOKS
}

# TODO: This var is set by mkspecs.
!isEmpty(EGLFS_DEVICE_INTEGRATION) {
    DEFINES += EGLFS_PREFERRED_PLUGIN=$$EGLFS_DEVICE_INTEGRATION
}

# Prevent gold linker from crashing.
# This started happening when QtPlatformSupport was modularized.
use_gold_linker: CONFIG += no_linker_version_script
*/

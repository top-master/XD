import qbs
import "uiautomation/uiautomation.qbs" as SrcUiAutomation

QtPlugin {
    name: "qwindows"
    condition: qbs.targetOS.contains("windows") && !targetsUWP
    pluginType: "platforms"
    pluginClassName: "QWindowsIntegrationPlugin"
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.eventdispatcher_support_private" }
    Depends { name: "Qt.accessibility_support_private" }
    Depends { name: "Qt.fontdatabase_support_private" }
    Depends { name: "Qt.theme_support_private" }
    Depends {
        name: "Qt.windowsuiautomation_support_private"
        condition: Qt.gui.config.accessibility
    }
    Depends {
        name: "qt_iaccessible2"
        condition: Qt.gui.config.accessibility && !qbs.toolchain.contains("mingw")
    }

    cpp.includePaths: {
        var result = base.concat(".");
        if (Qt.gui.config.tabletevent)
            result.push(qtbaseDir + "/src/3rdparty/wintab");
        return result;
    }
    cpp.defines: [
        "QT_NO_CAST_FROM_ASCII",
        "LIBEGL_NAME=" + Qt.gui.libEGLName,
        "LIBGLESV2_NAME=" + Qt.gui.libGLESv2Name,
    ]
    cpp.dynamicLibraries: {
        var result = ["gdi32", "dwmapi"];

        // Note: OpenGL32 must precede Gdi32 as it overwrites some functions.
        result.push("ole32", "user32", "winspool", "imm32", "winmm", "oleaut32");

        if (Qt.gui.config.opengl && !Qt.gui.config.opengles2 && !Qt.gui.config.dynamicgl)
            result.push("opengl32");

        if (qbs.toolchain.contains("mingw"))
            result.push("uuid");

        // For the dialog helpers:
        result.push("shlwapi", "shell32", "advapi32");
        return result;
    }
    files: [
        "main.cpp",
        "openglblacklists.qrc",
        "qtwindowsglobal.h",
        "qwin10helpers.cpp",
        "qwin10helpers.h",
        "qwindowsbackingstore.cpp",
        "qwindowsbackingstore.h",
        "qwindowscontext.cpp",
        "qwindowscontext.h",
        "qwindowscursor.cpp",
        "qwindowscursor.h",
        "qwindowsdialoghelpers.cpp",
        "qwindowsdialoghelpers.h",
        "qwindowsdropdataobject.cpp",
        "qwindowsdropdataobject.h",
        "qwindowsgdiintegration.cpp",
        "qwindowsgdiintegration.h",
        "qwindowsgdinativeinterface.cpp",
        "qwindowsgdinativeinterface.h",
        "qwindowsinputcontext.cpp",
        "qwindowsinputcontext.h",
        "qwindowsintegration.cpp",
        "qwindowsintegration.h",
        "qwindowsinternalmimedata.cpp",
        "qwindowsinternalmimedata.h",
        "qwindowskeymapper.cpp",
        "qwindowskeymapper.h",
        "qwindowsmenu.cpp",
        "qwindowsmenu.h",
        "qwindowsmime.cpp",
        "qwindowsmime.h",
        "qwindowsmousehandler.cpp",
        "qwindowsmousehandler.h",
        "qwindowsnativeinterface.cpp",
        "qwindowsnativeinterface.h",
        "qwindowsole.cpp",
        "qwindowsole.h",
        "qwindowsopengltester.cpp",
        "qwindowsopengltester.h",
        "qwindowsscreen.cpp",
        "qwindowsscreen.h",
        "qwindowsservices.cpp",
        "qwindowsservices.h",
        "qwindowstheme.cpp",
        "qwindowstheme.h",
        "qwindowsthreadpoolrunner.h",
        "qwindowswindow.cpp",
        "qwindowswindow.h",
        "windows.json",
    ]
    Group {
        condition: Qt.gui.config.opengl && !Qt.gui.config.dynamicgl && !Qt.gui.config.opengles2
        files: [
            "qwindowsglcontext.cpp",
            "qwindowsglcontext.h",
            "qwindowsopenglcontext.h",
        ]
    }
    Group {
        condition: Qt.gui.config.dynamicgl || Qt.gui.config.opengles2
        files: [
            "qwindowseglcontext.cpp",
            "qwindowseglcontext.h",
        ]
    }
    Group {
        condition: Qt.gui.config.clipboard
        files: [
            "qwindowsclipboard.cpp",
            "qwindowsclipboard.h",
        ]
    }
    Group {
        condition: Qt.gui.config.draganddrop && (product.targetsUWP || Qt.gui.config.clipboard)
        files: [
            "qwindowsdrag.cpp",
            "qwindowsdrag.h",
        ]
    }
    Group {
        condition: Qt.gui.config.tabletevent
        files: [
            "qwindowstabletsupport.cpp",
            "qwindowstabletsupport.h",
        ]
    }
    Group {
        condition: Qt.gui.config.sessionmanager
        files: [
            "qwindowssessionmanager.cpp",
            "qwindowssessionmanager.h",
        ]
    }
    Group {
        condition: Qt.gui.config.systemtrayicon
        files: [
            "qwindowssystemtrayicon.cpp",
            "qwindowssystemtrayicon.h",
        ]
    }
    Group {
        condition: Qt.gui_private.config.png
        files: [
            "cursors.qrc",
        ]
    }
    SrcUiAutomation { }
    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}

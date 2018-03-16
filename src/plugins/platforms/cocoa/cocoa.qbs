import qbs
import QtGlobalPrivateConfig

QtPlugin {
    name: "qcocoa"
    condition: qbs.targetOS.contains("macos") && QtGlobalPrivateConfig.gui
    pluginType: "platforms"
    pluginClassName: "QCocoaIntegrationPlugin"
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.widgets_private"; condition: QtGlobalPrivateConfig.widgets }
    Depends { name: "Qt.printsupport_private"; condition: QtGlobalPrivateConfig.widgets }
    Depends { name: "Qt.accessibility_support_private" }
    Depends { name: "Qt.clipboard_support_private" }
    Depends { name: "Qt.theme_support_private" }
    Depends { name: "Qt.fontdatabase_support_private" }
    Depends { name: "Qt.graphics_support_private" }
    Depends { name: "Qt.cgl_support_private" }

    cpp.requireAppExtensionSafeApi: false
    cpp.frameworks: ["AppKit", "Carbon", "IOKit"]
    cpp.dynamicLibraries: ["cups"]
    cpp.defines: base.filter(function(d) { return d !== "QT_NO_FOREACH" })
    files: [
        "cocoa.json",
        "main.mm",
        "messages.cpp",
        "messages.h",
        "qcocoaaccessibility.h",
        "qcocoaaccessibility.mm",
        "qcocoaaccessibilityelement.h",
        "qcocoaaccessibilityelement.mm",
        "qcocoaapplication.h",
        "qcocoaapplication.mm",
        "qcocoaapplicationdelegate.h",
        "qcocoaapplicationdelegate.mm",
        "qcocoabackingstore.h",
        "qcocoabackingstore.mm",
        "qcocoaclipboard.h",
        "qcocoaclipboard.mm",
        "qcocoacolordialoghelper.h",
        "qcocoacolordialoghelper.mm",
        "qcocoacursor.h",
        "qcocoacursor.mm",
        "qcocoadrag.h",
        "qcocoadrag.mm",
        "qcocoaeventdispatcher.h",
        "qcocoaeventdispatcher.mm",
        "qcocoafiledialoghelper.h",
        "qcocoafiledialoghelper.mm",
        "qcocoafontdialoghelper.h",
        "qcocoafontdialoghelper.mm",
        "qcocoahelpers.h",
        "qcocoahelpers.mm",
        "qcocoainputcontext.h",
        "qcocoainputcontext.mm",
        "qcocoaintegration.h",
        "qcocoaintegration.mm",
        "qcocoaintrospection.h",
        "qcocoaintrospection.mm",
        "qcocoakeymapper.h",
        "qcocoakeymapper.mm",
        "qcocoamenu.h",
        "qcocoamenu.mm",
        "qcocoamenubar.h",
        "qcocoamenubar.mm",
        "qcocoamenuitem.h",
        "qcocoamenuitem.mm",
        "qcocoamenuloader.h",
        "qcocoamenuloader.mm",
        "qcocoamimetypes.h",
        "qcocoamimetypes.mm",
        "qcocoanativeinterface.h",
        "qcocoanativeinterface.mm",
        "qcocoaresources.qrc",
        "qcocoaservices.h",
        "qcocoaservices.mm",
        "qcocoasystemsettings.h",
        "qcocoasystemsettings.mm",
        "qcocoasystemtrayicon.h",
        "qcocoasystemtrayicon.mm",
        "qcocoatheme.h",
        "qcocoatheme.mm",
        "qcocoawindow.h",
        "qcocoawindow.mm",
        "qmacclipboard.h",
        "qmacclipboard.mm",
        "qmultitouch_mac.mm",
        "qmultitouch_mac_p.h",
        "qnsview.h",
        "qnsview.mm",
        "qnsviewaccessibility.mm",
        "qnswindowdelegate.h",
        "qnswindowdelegate.mm",
    ]
    Group {
        condition: QtGuiConfig.opengl
        files: [
            "qcocoaglcontext.h",
            "qcocoaglcontext.mm",
        ]
    }
    Group {
        condition: QtGlobalPrivateConfig.widgets
        files: [
            "qcocoaprintdevice.h",
            "qcocoaprintdevice.mm",
            "qcocoaprintersupport.h",
            "qcocoaprintersupport.mm",
            "qpaintengine_mac.mm",
            "qpaintengine_mac_p.h",
            "qprintengine_mac.mm",
            "qprintengine_mac_p.h",
        ]
    }
    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}

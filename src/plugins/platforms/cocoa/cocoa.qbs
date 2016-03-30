import qbs

QtPlugin {
    condition: qbs.targetOS.contains("osx")
    category: "platforms"
    name: "qcocoa"

    readonly property string prefix: project.qtbasePrefix + "src/plugins/platforms/cocoa/"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }

    Depends { name: "Qt.widgets"; condition: project.widgets }
    Depends { name: "Qt.widgets-private"; condition: project.widgets }
    Depends { name: "Qt.printsupport"; condition: project.printsupport }
    Depends { name: "Qt.printsupport-private"; condition: project.printsupport }

    cpp.dynamicLibraries: (project.cups ? [
        "cups"
    ] : []).concat(base)

    cpp.frameworks: [
        "AppKit", "Carbon", "IOKit"
    ].concat(base)

    files: [
        "qcocoaresources.qrc",
        "cocoa.json",
    ]

    Group {
        name: "headers"
        prefix: product.prefix
        files: [
            "messages.h",
            "qcocoaaccessibility.h",
            "qcocoaaccessibilityelement.h",
            "qcocoaapplication.h",
            "qcocoaapplicationdelegate.h",
            "qcocoabackingstore.h",
            "qcocoaclipboard.h",
            "qcocoacolordialoghelper.h",
            "qcocoacursor.h",
            "qcocoadrag.h",
            "qcocoaeventdispatcher.h",
            "qcocoafiledialoghelper.h",
            "qcocoafontdialoghelper.h",
            "qcocoahelpers.h",
            "qcocoainputcontext.h",
            "qcocoaintegration.h",
            "qcocoaintrospection.h",
            "qcocoakeymapper.h",
            "qcocoamenu.h",
            "qcocoamenubar.h",
            "qcocoamenuitem.h",
            "qcocoamenuloader.h",
            "qcocoamimetypes.h",
            "qcocoanativeinterface.h",
            "qcocoaservices.h",
            "qcocoasystemsettings.h",
            "qcocoasystemtrayicon.h",
            "qcocoatheme.h",
            "qcocoawindow.h",
            "qmacclipboard.h",
            "qmultitouch_mac_p.h",
            "qnsview.h",
            "qnswindowdelegate.h",
        ]
    }

    Group {
        name: "sources"
        prefix: product.prefix
        files: [
            "main.mm",
            "messages.cpp",
            "qcocoaaccessibility.mm",
            "qcocoaaccessibilityelement.mm",
            "qcocoaapplication.mm",
            "qcocoaapplicationdelegate.mm",
            "qcocoabackingstore.mm",
            "qcocoaclipboard.mm",
            "qcocoacolordialoghelper.mm",
            "qcocoacursor.mm",
            "qcocoadrag.mm",
            "qcocoaeventdispatcher.mm",
            "qcocoafiledialoghelper.mm",
            "qcocoafontdialoghelper.mm",
            "qcocoahelpers.mm",
            "qcocoainputcontext.mm",
            "qcocoaintegration.mm",
            "qcocoaintrospection.mm",
            "qcocoakeymapper.mm",
            "qcocoamenu.mm",
            "qcocoamenubar.mm",
            "qcocoamenuitem.mm",
            "qcocoamenuloader.mm",
            "qcocoamimetypes.mm",
            "qcocoanativeinterface.mm",
            "qcocoaservices.mm",
            "qcocoasystemsettings.mm",
            "qcocoasystemtrayicon.mm",
            "qcocoatheme.mm",
            "qcocoawindow.mm",
            "qmacclipboard.mm",
            "qmultitouch_mac.mm",
            "qnsview.mm",
            "qnsviewaccessibility.mm",
            "qnswindowdelegate.mm",
        ]
    }

    Group {
        condition: project.opengl
        name: "OpenGL"
        prefix: product.prefix
        files: [
            "qcocoaglcontext.h",
            "qcocoaglcontext.mm",
        ]
    }

    Group {
        condition: project.widgets && project.printsupport
        name: "Widgets"
        prefix: product.prefix
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
}

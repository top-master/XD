import qbs

QtPlugin {
    id: root
    condition: qbs.targetOS.contains("android")
    category: "platforms/android"
    name: "qtforandroid"

    readonly property string prefix: project.qtbasePrefix + "src/plugins/platforms/android/"

    Depends { name: "Qt"; submodules: ["core", "core-private", "gui", "gui-private", "platformsupport-private"] }

    cpp.defines: [
        "QT_STATICPLUGIN",
    ].concat(base)

    cpp.includePaths: [
        root.prefix,
    ].concat(base)

    Group {
        name: "sources"
        prefix: root.prefix
        files: [
            "androiddeadlockprotector.cpp",
            "androiddeadlockprotector.h",
            "androidjniaccessibility.cpp",
            "androidjniaccessibility.h",
            "androidjniclipboard.cpp",
            "androidjniclipboard.h",
            "androidjniinput.cpp",
            "androidjniinput.h",
            "androidjnimain.cpp",
            "androidjnimain.h",
            "androidjnimenu.cpp",
            "androidjnimenu.h",
            "androidplatformplugin.cpp",
            "androidsurfaceclient.h",
            "qandroidassetsfileenginehandler.cpp",
            "qandroidassetsfileenginehandler.h",
            "qandroideventdispatcher.cpp",
            "qandroideventdispatcher.h",
            "qandroidinputcontext.cpp",
            "qandroidinputcontext.h",
            "qandroidplatformaccessibility.cpp",
            "qandroidplatformaccessibility.h",
            "qandroidplatformbackingstore.cpp",
            "qandroidplatformbackingstore.h",
            "qandroidplatformclipboard.cpp",
            "qandroidplatformclipboard.h",
            "qandroidplatformdialoghelpers.cpp",
            "qandroidplatformdialoghelpers.h",
            "qandroidplatformfontdatabase.cpp",
            "qandroidplatformfontdatabase.h",
            "qandroidplatformforeignwindow.cpp",
            "qandroidplatformforeignwindow.h",
            "qandroidplatformintegration.cpp",
            "qandroidplatformintegration.h",
            "qandroidplatformmenubar.cpp",
            "qandroidplatformmenubar.h",
            "qandroidplatformmenu.cpp",
            "qandroidplatformmenu.h",
            "qandroidplatformmenuitem.cpp",
            "qandroidplatformmenuitem.h",
            "qandroidplatformopenglcontext.cpp",
            "qandroidplatformopenglcontext.h",
            "qandroidplatformopenglwindow.cpp",
            "qandroidplatformopenglwindow.h",
            "qandroidplatformscreen.cpp",
            "qandroidplatformscreen.h",
            "qandroidplatformservices.cpp",
            "qandroidplatformservices.h",
            "qandroidplatformtheme.cpp",
            "qandroidplatformtheme.h",
            "qandroidplatformwindow.cpp",
            "qandroidplatformwindow.h",
            "qandroidsystemlocale.cpp",
            "qandroidsystemlocale.h",
        ]
    }
}

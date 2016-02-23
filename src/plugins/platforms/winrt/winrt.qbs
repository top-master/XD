import qbs

QtPlugin {
    condition: qbs.targetOS.contains("winrt")
    category: "platforms"
    name: "qwinrt"

    readonly property string prefix: project.qtbasePrefix + "src/plugins/platforms/winrt/"

    Depends { name: "egl" }
    Depends { name: "glesv2" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }

    cpp.dynamicLibraries: [
        "dwrite",
        "runtimeobject",
    ].concat(base)

    Group {
        name: "headers"
        prefix: product.prefix
        files: [
            "qwinrtbackingstore.h",
            "qwinrtcursor.h",
            "qwinrteglcontext.h",
            "qwinrteventdispatcher.h",
            "qwinrtfiledialoghelper.h",
            "qwinrtfileengine.h",
            "qwinrtfontdatabase.h",
            "qwinrtinputcontext.h",
            "qwinrtintegration.h",
            "qwinrtmessagedialoghelper.h",
            "qwinrtscreen.h",
            "qwinrtservices.h",
            "qwinrttheme.h",
            "qwinrtwindow.h",
        ]
    }

    Group {
        name: "sources"
        prefix: product.prefix
        files: [
            "main.cpp",
            "qwinrtbackingstore.cpp",
            "qwinrtcursor.cpp",
            "qwinrteglcontext.cpp",
            "qwinrteventdispatcher.cpp",
            "qwinrtfiledialoghelper.cpp",
            "qwinrtfileengine.cpp",
            "qwinrtfontdatabase.cpp",
            "qwinrtinputcontext.cpp",
            "qwinrtintegration.cpp",
            "qwinrtmessagedialoghelper.cpp",
            "qwinrtscreen.cpp",
            "qwinrtservices.cpp",
            "qwinrttheme.cpp",
            "qwinrtwindow.cpp",
        ]
    }
}

Group {
    prefix: path + "/"
    files: [
        "qcolormap.cpp",
        "qcolormap.h",
        "qcompleter.cpp",
        "qcompleter.h",
        "qcompleter_p.h",
        "qsystemtrayicon.cpp",
        "qsystemtrayicon.h",
        "qsystemtrayicon_p.h",
        "qundogroup.cpp",
        "qundogroup.h",
        "qundostack.cpp",
        "qundostack.h",
        "qundostack_p.h",
        "qundoview.cpp",
        "qundoview.h",
    ]

    Group {
        condition: product.config.scroller
        files: [
            "qflickgesture.cpp",
            "qflickgesture_p.h",
            "qscroller.cpp",
            "qscroller.h",
            "qscroller_p.h",
            "qscrollerproperties.cpp",
            "qscrollerproperties.h",
            "qscrollerproperties_p.h",
        ]
    }
    Group {
        condition: product.Qt.gui_private.config.xcb
        files: [
            "qsystemtrayicon_x11.cpp",
        ]
    }
    Group {
        condition: !product.Qt.gui_private.config.xcb
        files: [
            "qsystemtrayicon_qpa.cpp",
        ]
    }
    Group {
        condition: qbs.targetOS.contains("macos") && product.config.scroller
        files: [
            "qscroller_mac.mm",
        ]
    }
}

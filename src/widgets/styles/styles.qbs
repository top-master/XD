Group {
    prefix: path + "/"
    files: [
        "qcommonstyle.cpp",
        "qcommonstyle.h",
        "qcommonstyle_p.h",
        "qcommonstylepixmaps_p.h",
        "qdrawutil.cpp",
        "qdrawutil.h",
        "qpixmapstyle.cpp",
        "qpixmapstyle_p.h",
        "qpixmapstyle_p_p.h",
        "qproxystyle.cpp",
        "qproxystyle.h",
        "qproxystyle_p.h",
        "qstyle.cpp",
        "qstyle.h",
        "qstyle.qrc",
        "qstyleanimation.cpp",
        "qstyleanimation_p.h",
        "qstylefactory.cpp",
        "qstylefactory.h",
        "qstylehelper.cpp",
        "qstylehelper_p.h",
        "qstyleoption.cpp",
        "qstyleoption.h",
        "qstylepainter.cpp",
        "qstylepainter.h",
        "qstyleplugin.cpp",
        "qstyleplugin.h",
        "qstylesheetstyle.cpp",
        "qstylesheetstyle_default.cpp",
        "qstylesheetstyle_p.h",
    ]
    Group {
        condition: product.privateConfig.style_fusion
        files: [
            "qfusionstyle.cpp",
            "qfusionstyle_p.h",
            "qfusionstyle_p_p.h",
        ]
    }
    Group {
        condition: product.privateConfig.style_windows
        files: [
            "qwindowsstyle.cpp",
            "qwindowsstyle_p.h",
            "qwindowsstyle_p_p.h",
        ]
    }
}

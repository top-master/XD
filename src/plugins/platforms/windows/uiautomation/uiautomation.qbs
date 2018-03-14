import qbs

Group {
    condition: Qt.gui.config.accessibility
    prefix: path + "/"
    files: [
        "qwindowsuiaaccessibility.cpp",
        "qwindowsuiaaccessibility.h",
        "qwindowsuiabaseprovider.cpp",
        "qwindowsuiabaseprovider.h",
        "qwindowsuiagriditemprovider.cpp",
        "qwindowsuiagriditemprovider.h",
        "qwindowsuiagridprovider.cpp",
        "qwindowsuiagridprovider.h",
        "qwindowsuiainvokeprovider.cpp",
        "qwindowsuiainvokeprovider.h",
        "qwindowsuiamainprovider.cpp",
        "qwindowsuiamainprovider.h",
        "qwindowsuiaprovidercache.cpp",
        "qwindowsuiaprovidercache.h",
        "qwindowsuiarangevalueprovider.cpp",
        "qwindowsuiarangevalueprovider.h",
        "qwindowsuiaselectionitemprovider.cpp",
        "qwindowsuiaselectionitemprovider.h",
        "qwindowsuiaselectionprovider.cpp",
        "qwindowsuiaselectionprovider.h",
        "qwindowsuiatableitemprovider.cpp",
        "qwindowsuiatableitemprovider.h",
        "qwindowsuiatableprovider.cpp",
        "qwindowsuiatableprovider.h",
        "qwindowsuiatextprovider.cpp",
        "qwindowsuiatextprovider.h",
        "qwindowsuiatextrangeprovider.cpp",
        "qwindowsuiatextrangeprovider.h",
        "qwindowsuiatoggleprovider.cpp",
        "qwindowsuiatoggleprovider.h",
        "qwindowsuiautils.cpp",
        "qwindowsuiautils.h",
        "qwindowsuiavalueprovider.cpp",
        "qwindowsuiavalueprovider.h",
    ]
}

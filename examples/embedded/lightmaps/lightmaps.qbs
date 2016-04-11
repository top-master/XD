import qbs

QtExample {
    name: "lightmaps"
    condition: Qt.network.present && Qt.widgets.present
    install: false
    Depends { name: "Qt.network"; required: false }
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "lightmaps.cpp",
        "lightmaps.h",
        "main.cpp",
        "mapzoom.cpp",
        "mapzoom.h",
        "slippymap.cpp",
        "slippymap.h",
    ]
}

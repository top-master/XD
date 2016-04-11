import qbs

QtExample {
    name: "painterpaths"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "renderarea.cpp",
        "renderarea.h",
        "window.cpp",
        "window.h",
    ]

    Properties {
        condition: qbs.targetOS.contains("unix")
                   && !qbs.targetOS.contains("darwin")
                   && !qbs.targetOS.contains("vxworks")
                   && !qbs.targetOS.contains("integrity")
                   && !qbs.targetOS.contains("haiku")
        cpp.dynamicLibraries: base.concat("m")
    }
}

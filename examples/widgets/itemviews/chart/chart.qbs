import qbs

QtExample {
    name: "chart"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "chart.qrc",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "pieview.cpp",
        "pieview.h",
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

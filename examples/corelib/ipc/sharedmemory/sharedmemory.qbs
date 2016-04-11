import qbs

QtExample {
    name: "sharedmemory"
    condition: Qt.widgets.present && !qbs.targetOS.contains("vxworks")
               && !project.disabledFeatures.contains("sharedmemory")
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "dialog.cpp",
        "dialog.h",
        "dialog.ui",
        "main.cpp",
        "*.png",
    ]
}

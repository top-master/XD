import qbs

Project {
    references: [
        "eglfs/eglfs.qbs",
        "xcb/xcb.qbs",
        //"windows/windows.qbs",
        "winrt/winrt.qbs",
        "android/android.qbs",
        "cocoa/cocoa.qbs",
    ]
}

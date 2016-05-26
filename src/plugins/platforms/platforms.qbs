import qbs

Project {
    condition: project.gui
    references: [
        "eglfs/eglfs.qbs",
        "linuxfb/linuxfb.qbs",
        "minimal/minimal.qbs",
        "minimalegl/minimalegl.qbs",
        "offscreen/offscreen.qbs",
        "xcb/xcb.qbs",
        //"windows/windows.qbs",
        "winrt/winrt.qbs",
        "android/android.qbs",
        "cocoa/cocoa.qbs",
    ]
}

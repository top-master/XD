import qbs

Project {
    references: [
        // "android",
        // "bsdfb",
        // "cocoa",
        // "direct2d",
        // "directfb",
        "eglfs/eglfs.qbs",
        // "haiku",
        // "integrity",
        // "ios",
        // "linuxfb",
        "minimal",
        "minimalegl",
        // "mirclient",
        // "offscreen",
        // "qnx",
        // "vnc",
        // "windows",
        // "winrt",
        "xcb",
        ]
}
/*
TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

android: SUBDIRS += android

!android:qtConfig(freetype): SUBDIRS += offscreen

uikit:!watchos: SUBDIRS += ios
osx: SUBDIRS += cocoa

win32:!winrt: SUBDIRS += windows
winrt: SUBDIRS += winrt

qtConfig(direct2d) {
    SUBDIRS += direct2d
}

qnx {
    SUBDIRS += qnx
}

qtConfig(directfb) {
    SUBDIRS += directfb
}

qtConfig(linuxfb): SUBDIRS += linuxfb

qtConfig(vnc): SUBDIRS += vnc

freebsd {
    SUBDIRS += bsdfb
}

haiku {
    SUBDIRS += haiku
}

qtConfig(mirclient): SUBDIRS += mirclient

qtConfig(integrityfb): SUBDIRS += integrity
*/

import qbs

Project {
    references: [
        "evdevkeyboard",
        "evdevmouse",
        "evdevtablet",
        "evdevtouch",
        "libinput",
        "tuiotouch",
    ]
}

/*
qtConfig(tslib) {
    SUBDIRS += tslib
}

freebsd {
    SUBDIRS += bsdkeyboard bsdmouse
}
*/

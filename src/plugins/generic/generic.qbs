import qbs
import QtCoreConfig

Project {
    condition: QtCoreConfig.library
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

import qbs

Project {
    condition: project.gui && !project.disabledFeatures.contains("library")

    references: [
        "evdevkeyboard",
        "evdevmouse",
        "evdevtablet",
        "evdevtouch",
        "libinput",
        "tslib",
        "tuiotouch",
    ]
}

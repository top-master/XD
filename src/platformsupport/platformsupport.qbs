import qbs

Project {
    references: [
        "accessibility",
        "clipboard",
        "devicediscovery",
        "edid",
        "eglconvenience",
        "eventdispatchers",
        "fbconvenience",
        "fontdatabases",
        "glxconvenience",
        "graphics",
        "input",
        "kmsconvenience",
        "linuxaccessibility",
        "platformcompositor",
        "services",
        "themes",
        "windowsuiautomation",
    ]
}

/*
qtConfig(vulkan): \
    SUBDIRS += vkconvenience
*/

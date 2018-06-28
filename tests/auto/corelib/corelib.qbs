import qbs

QtAutotestProject {
    name: "corelib_tests"

    references: [
        "animation",
        "codecs",
        "global",
        "io",
        "itemmodels",
        "kernel",
        "mimetypes",
        // "plugin",
        // "statemachine",
        // "thread",
        // "tools",
        // "xml",
    ]
}

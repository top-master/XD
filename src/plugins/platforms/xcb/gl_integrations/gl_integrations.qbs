import qbs

Project {
    qbsSearchPaths: [path]
    references: [
        "xcb_egl",
        "xcb_glx",
    ]
}

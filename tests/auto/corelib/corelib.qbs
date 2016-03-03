import qbs

QtAutotestProject {
    name: "corelib_tests"

    references: [
        "kernel",
    ]

// !ios: SUBDIRS += \
//    animation \
//    codecs \
//    global \
//    io \
//    itemmodels \
//    json \
//    mimetypes \
//    plugin \
//    statemachine \
//    thread \
//    tools \
//    xml
}

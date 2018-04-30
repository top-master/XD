import qbs

QtAutotestProject {
    name: "corelib_codecs_tests"
    references: [
        "qtextcodec",
        "utf8", // Integrate into qtextcodec test? See QTBUG-22590.
    ]
}

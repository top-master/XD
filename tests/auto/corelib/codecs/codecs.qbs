import qbs

QtAutotestProject {
    name: "corelib_codecs_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "qtextcodec",
        "utf8", // Integrate into qtextcodec test? See QTBUG-22590.
    ]
}

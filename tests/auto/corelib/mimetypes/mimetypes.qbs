import qbs

QtAutotestProject {
    name: "corelib_mimetype_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "qmimedatabase",
        "qmimetype",
    ]
}

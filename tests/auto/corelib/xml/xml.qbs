import qbs

QtAutotestProject {
    name: "corelib_xml_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "qxmlstream",
    ]
}


import qbs

QtAutotestProject {
    name: "corelib_statemachine_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "qstate",
        "qstatemachine",
    ]
}

import qbs

QtAutotestProject {
    name: "corelib_animation_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "qabstractanimation",
        "qanimationgroup",
        "qparallelanimationgroup",
        "qpauseanimation",
        "qpropertyanimation",
        "qsequentialanimationgroup",
        "qvariantanimation",
    ]
}

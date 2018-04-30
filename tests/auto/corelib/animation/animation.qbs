import qbs

QtAutotestProject {
    name: "corelib_animation_tests"
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

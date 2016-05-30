import qbs

QtAutotestProject {
    name: "corelib_global_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "q_func_info",
        "qflags",
        "qgetputenv",
        "qglobal",
        "qglobalstatic",
        "qhooks",
        "qlogging",
        "qnumeric",
        "qrand",
        "qtendian",
    ]
}

import qbs

QtAutotestProject {
    name: "corelib_global_tests"
    references: [
        "q_func_info",
        "qflags",
        "qgetputenv",
        "qglobal",
        "qglobalstatic",
        "qfloat16",
        "qhooks",
        "qlogging",
        "qnumeric",
        "qrand",
        "qrandomgenerator",
        "qtendian",
    ]
}

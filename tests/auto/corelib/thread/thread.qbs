import qbs

QtAutotestProject {
    name: "corelib_thread_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "qatomicint",
        "qatomicinteger",
        "qatomicpointer",
        "qfuture",
        "qfuturesynchronizer",
        "qfuturewatcher",
        "qmutex",
        "qmutexlocker",
        "qreadlocker",
        "qreadwritelock",
        "qresultstore",
        "qsemaphore",
        "qthread",
        "qthreadonce",
        "qthreadpool",
        "qthreadstorage",
        "qwaitcondition",
        "qwritelocker",
    ]
}

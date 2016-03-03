import qbs

QtAutotestProject {
    name: "corelib_kernel_tests"
    references: [
        "qcoreapplication",
        "qeventdispatcher",
        "qeventloop",
        "qmath",
        "qmetaenum",
        "qmetamethod",
        "qmetaobject",
        "qmetaobjectbuilder",
        "qmetaproperty",
        "qmetatype",
        "qmimedata",
        "qobject",
        "qpointer",
        "qsharedmemory",
        "qsignalblocker",
        "qsignalmapper",
        "qsocketnotifier",
        "qsystemsemaphore",
        "qtimer",
        "qtranslator",
        "qvariant",
        "qwineventnotifier",
    ]
}

import qbs

Group {
    name: "plugin"
    prefix: path + '/'
    files: [
        "qfactoryinterface.cpp",
        "qfactoryinterface.h",
        "qfactoryloader.cpp",
        "qfactoryloader_p.h",
        "qpluginloader.cpp",
        "qpluginloader.h",
        "qplugin.h",
        "quuid.cpp",
        "quuid.h",
    ]

    Group {
        name: "plugin/windows"
        condition: qbs.targetOS.contains("windows")
        files: [
            "qsystemlibrary.cpp",
            "qsystemlibrary_p.h",
        ]
    }
    Group {
        name: "plugin/library"
        condition: product.config.library
        files: [
            "qelfparser_p.cpp",
            "qelfparser_p.h",
            "qlibrary.cpp",
            "qlibrary.h",
            "qlibrary_p.h",
            "qmachparser.cpp",
            "qmachparser_p.h",
        ]

        Group {
            name: "plugin/library/unix"
            condition: qbs.targetOS.contains("unix")
            files: ["qlibrary_unix.cpp"]
        }
        Group {
            name: "plugin/library/windows"
            condition: qbs.targetOS.contains("windows")
            files: ["qlibrary_win.cpp"]
        }
    }
}

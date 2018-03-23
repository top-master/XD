import qbs

QtModuleProject {
    name: "QtConcurrent"
    simpleName: "concurrent"
    conditionFunction: (function() {
        return Qt.global.config.concurrent;
    })

    QtHeaders {
        sync.classNames: ({
            "qtconcurrentfilter.h": ["QtConcurrentFilter"],
            "qtconcurrentmap.h": ["QtConcurrentMap"],
            "qtconcurrentrun.h": ["QtConcurrentRun"],
        })
        Depends { name: "QtCoreHeaders" }
    }

    QtPrivateModule {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt"; submodules: ["core-private"] }

        cpp.defines: base.concat([
            "QT_NO_FOREACH",
            "QT_NO_USING_NAMESPACE",
        ])
        cpp.enableExceptions: true
        cpp.includePaths: project.includePaths.concat(base)

        Properties {
            condition: qbs.targetOS.contains("windows") && qbs.toolchain.contains("msvc")
            cpp.linkerFlags: base.concat("/BASE:0x66000000")
        }

        // Properties {
        //     condition: project.config.contains("clock-gettime") &&
        //                (qbs.targetOS.contains("linux")
        //                 || qbs.targetOS.contains("hpux")
        //                 || qbs.targetOS.contains("solaris"))
        //     cpp.dynamicLibraries: base.concat("rt")
        // }

        files: [
            "qtconcurrent_global.h",
            "qtconcurrentcompilertest.h",
            "qtconcurrentexception.h",
            "qtconcurrentfilter.cpp",
            "qtconcurrentfilter.h",
            "qtconcurrentfilterkernel.h",
            "qtconcurrentfunctionwrappers.h",
            "qtconcurrentiteratekernel.cpp",
            "qtconcurrentiteratekernel.h",
            "qtconcurrentmap.cpp",
            "qtconcurrentmap.h",
            "qtconcurrentmapkernel.h",
            "qtconcurrentmedian.h",
            "qtconcurrentreducekernel.h",
            "qtconcurrentrun.cpp",
            "qtconcurrentrun.h",
            "qtconcurrentrunbase.h",
            "qtconcurrentstoredfunctioncall.h",
            "qtconcurrentthreadengine.cpp",
            "qtconcurrentthreadengine.h",
        ]

        Group {
            name: "Qt.core precompiled header"
            files: ["../corelib/global/qt_pch.h"]
        }
    }
}

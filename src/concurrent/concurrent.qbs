import qbs
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtConcurrent"
    simpleName: "concurrent"
    prefix: project.qtbasePrefix + "src/concurrent/"

    Product {
        name: root.privateName
        condition: project.concurrent
        profiles: project.targetProfiles
        Depends { name: root.moduleName }
        Export {
            Depends { name: "cpp" }
            cpp.defines: root.defines
            cpp.includePaths: root.includePaths
        }
    }

    QtHeaders {
        name: root.headersName
        sync.module: root.name
        sync.classNames: ({
            "qtconcurrentfilter.h": ["QtConcurrentFilter"],
            "qtconcurrentmap.h": ["QtConcurrentMap"],
            "qtconcurrentrun.h": ["QtConcurrentRun"],
        })
        Depends { name: "QtCoreHeaders" }
        ModuleHeaders { fileTags: ["hpp_syncable"] }
    }

    QtModule {
        name: root.moduleName
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "concurrent.pro"

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends { name: "Qt"; submodules: ["core", "core-private"] }

        cpp.defines: [
            "QT_BUILD_CONCURRENT_LIB", // TODO: Put this in the QtModule item
            "QT_NO_USING_NAMESPACE"
        ]

        cpp.includePaths: root.includePaths.concat(base)

        Properties {
            condition: qbs.targetOS.contains("windows")
                       && (qbs.toolchain.contains("msvc") || qbs.toolchain.contains("icc"))
            cpp.linkerFlags: base.concat("/BASE:0x66000000")
        }

        Properties {
            condition: project.config.contains("clock-gettime") &&
                       (qbs.targetOS.contains("linux")
                        || qbs.targetOS.contains("hpux")
                        || qbs.targetOS.contains("solaris"))
            cpp.dynamicLibraries: base.concat("rt")
        }

        Group {
            name: "corelib_pch"
            files: [project.corelibPrecompiledHeader]
        }
        cpp.useCxxPrecompiledHeader: project.precompiledHeaders

        ModuleHeaders { }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "qtconcurrentfilter.cpp",
                "qtconcurrentiteratekernel.cpp",
                "qtconcurrentmap.cpp",
                "qtconcurrentrun.cpp",
                "qtconcurrentthreadengine.cpp",
            ]
        }
    }
}

// CONFIG    += exceptions
// QMAKE_DOCS = $$PWD/doc/qtconcurrent.qdocconf

import qbs
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtTest"
    simpleName: "testlib"
    prefix: project.qtbasePrefix + "src/testlib/"

    Product {
        name: root.privateName
        profiles: project.targetProfiles
        type: "hpp"
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
            "qtest.h": ["QTest"],
        })
        Depends { name: "QtCoreHeaders" }
        ModuleHeaders { fileTags: "hpp_syncable" }
    }

    QtModule {
        name: root.moduleName
        condition: project.testlib
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "testlib.pro"

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends { name: "Qt.core" }
        Depends { name: "Qt.core-private" }

        cpp.defines: [
            "QT_BUILD_TESTLIB_LIB",
        ].concat(base)

        cpp.includePaths: root.includePaths.concat(base)

        cpp.frameworks: {
            var frameworks = base;
            if (qbs.targetOS.contains("darwin")) {
                frameworks.push("CoreFoundation", "IOKit");
            }
            return frameworks;
        }

        ModuleHeaders { }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "qabstracttestlogger.cpp",
                "qasciikey.cpp",
                "qbenchmark.cpp",
                "qbenchmarkevent.cpp",
                "qbenchmarkmeasurement.cpp",
                "qbenchmarkmetric.cpp",
                "qbenchmarkperfevents.cpp",
                "qbenchmarkvalgrind.cpp",
                "qcsvbenchmarklogger.cpp",
                "qplaintestlogger.cpp",
                "qsignaldumper.cpp",
                "qteamcitylogger.cpp",
                "qtestblacklist.cpp",
                "qtestcase.cpp",
                "qtestdata.cpp",
                "qtestelement.cpp",
                "qtestelementattribute.cpp",
                "qtestlog.cpp",
                "qtestmouse.cpp",
                "qtestresult.cpp",
                "qtesttable.cpp",
                "qtestxunitstreamer.cpp",
                "qxmltestlogger.cpp",
                "qxunittestlogger.cpp",
            ]
        }
    }
}

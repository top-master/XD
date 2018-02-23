import qbs

QtModuleProject {
    name: "QtTest"
    simpleName: "testlib"
    conditionFunction: (function() { return QtGlobalPrivateConfig.testlib; })

    QtHeaders {
        sync.classNames: ({
            "qtest.h": ["QTest"],
        })
        Depends { name: "QtCoreHeaders" }
        Depends { name: "QtGlobalPrivateConfig" }
    }

    QtPrivateModule {
        Depends { name: "QtGlobalPrivateConfig" }
    }

    QtModule {
        name: project.moduleName
        simpleName: project.simpleName

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core-private" }
        Depends { name: "QtGlobalPrivateConfig" }

        cpp.enableExceptions: true
        cpp.includePaths: project.includePaths.concat(base)

        cpp.frameworks: {
            var frameworks = base;
            if (qbs.targetOS.contains("darwin"))
                frameworks.push("Security");
            if (qbs.targetOS.contains("macos"))
                frameworks.push("ApplicationServices", "IOKit", "Foundation");
            return frameworks;
        }

        files: [
            "qabstracttestlogger.cpp",
            "qasciikey.cpp",
            "qbenchmark.cpp",
            "qbenchmark.h",
            "qbenchmark_p.h",
            "qbenchmarkevent.cpp",
            "qbenchmarkevent_p.h",
            "qbenchmarkmeasurement.cpp",
            "qbenchmarkmeasurement_p.h",
            "qbenchmarkmetric.cpp",
            "qbenchmarkmetric.h",
            "qbenchmarkmetric_p.h",
            "qbenchmarkperfevents.cpp",
            "qbenchmarkperfevents_p.h",
            "qbenchmarktimemeasurers_p.h",
            "qbenchmarkvalgrind.cpp",
            "qbenchmarkvalgrind_p.h",
            "qcsvbenchmarklogger.cpp",
            "qplaintestlogger.cpp",
            "qsignaldumper.cpp",
            "qsignalspy.h",
            "qteamcitylogger.cpp",
            "qtest.h",
            "qtest_global.h",
            "qtest_gui.h",
            "qtest_network.h",
            "qtest_widgets.h",
            "qtestaccessible.h",
            "qtestassert.h",
            "qtestblacklist.cpp",
            "qtestblacklist_p.h",
            "qtestcase.cpp",
            "qtestcase.h",
            "qtestdata.cpp",
            "qtestdata.h",
            "qtestelement.cpp",
            "qtestelementattribute.cpp",
            "qtestevent.h",
            "qtesteventloop.h",
            "qtestkeyboard.h",
            "qtestlog.cpp",
            "qtestmouse.cpp",
            "qtestmouse.h",
            "qtestresult.cpp",
            "qtestspontaneevent.h",
            "qtestsystem.h",
            "qtesttable.cpp",
            "qtesttouch.h",
            "qtestxunitstreamer.cpp",
            "qxmltestlogger.cpp",
            "qxunittestlogger.cpp",
        ]

        Group {
            condition: qbs.targetOS.contains("macos")
            files: [
                "qtestutil_macos.mm",
                "qtestutil_macos_p.h",
            ]
        }
    }
}

/*
  // TODO

embedded:QMAKE_CXXFLAGS += -fno-rtti

# Exclude these headers from the clean check if their dependencies aren't
# being built
!qtHaveModule(gui) {
    HEADERSCLEAN_EXCLUDE += qtest_gui.h \
        qtestaccessible.h \
        qtestkeyboard.h \
        qtestmouse.h \
        qtesttouch.h
}

!qtHaveModule(widgets): HEADERSCLEAN_EXCLUDE += qtest_widgets.h

!qtHaveModule(network): HEADERSCLEAN_EXCLUDE += qtest_network.h
*/

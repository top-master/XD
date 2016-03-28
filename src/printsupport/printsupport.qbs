import qbs
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtPrintSupport"
    simpleName: "printsupport"
    prefix: project.qtbasePrefix + "src/printsupport/"

    defines: {
        var defines = base;
        if (!project.cups)
            defines.push("QT_NO_CUPS");
        return defines;
    }

    Product {
        name: root.privateName
        condition: project.printsupport
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
        condition: project.printsupport
        sync.module: root.name
        Depends { name: "QtCoreHeaders" }
        Depends { name: "QtGuiHeaders" }
        Depends { name: "QtWidgetsHeaders" }
        ModuleHeaders { fileTags: ["hpp_syncable"] }
    }

    QtModule {
        name: root.moduleName
        condition: project.printsupport
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "printsupport.pro"

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends {
            name: "Qt"
            submodules: ["core", "core-private", "gui", "gui-private", "widgets", "widgets-private"]
        }

        cpp.defines: [
            "QT_BUILD_PRINTSUPPORT_LIB",
            "QT_NO_USING_NAMESPACE"
        ].concat(base).concat(root.defines)

        cpp.includePaths: {
            var paths = root.includePaths.concat(base);
            paths.push(path + "/kernel", path + "/dialogs");
            if (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")) {
                paths.push(project.qtbasePrefix + "/src/plugins/printsupport/cups");
                if (project.cups)
                    paths.push(path + "/widgets");
            }
            return paths;
        }

        Properties {
            condition: qbs.targetOS.contains("windows")
            cpp.dynamicLibraries: base.concat("winspool", "comdlg32", "gdi32", "user32")
        }
        Properties {
            condition: qbs.targetOS.contains("osx")
            cpp.frameworks: base.concat("AppKit")
        }

        ModuleHeaders { }

        Group {
            name: "sources"
            files: [
                "dialogs/qabstractprintdialog.cpp",
                "dialogs/qpagesetupdialog.cpp",
                "dialogs/qpagesetupwidget.ui",
                "dialogs/qprintdialog.qrc",
                "dialogs/qprintpreviewdialog.cpp",
                "kernel/qpaintengine_alpha.cpp",
                "kernel/qpaintengine_preview.cpp",
                "kernel/qplatformprintdevice.cpp",
                "kernel/qplatformprintersupport.cpp",
                "kernel/qplatformprintplugin.cpp",
                "kernel/qprintdevice.cpp",
                "kernel/qprintengine_pdf.cpp",
                "kernel/qprinter.cpp",
                "kernel/qprinterinfo.cpp",
                "widgets/qprintpreviewwidget.cpp"
            ]
        }

        Group {
            name: "CUPS-specific sources"
            condition: project.cups
            files: [
                "widgets/qcupsjobwidget.cpp",
                "widgets/qcupsjobwidget.ui",
            ]
        }

        Group {
            name: "CUPS-specific sources (non-Apple)"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
                       && project.cups
            files: [
                "kernel/qcups.cpp"
            ]
        }

        Group {
            name: "Windows-specific sources"
            condition: qbs.targetOS.contains("windows")
            files: [
                "dialogs/qpagesetupdialog_win.cpp",
                "dialogs/qprintdialog_win.cpp",
                "kernel/qprintengine_win.cpp",
            ]
        }

        Group {
            name: "OS X-specific sources"
            condition: qbs.targetOS.contains("osx")
            files: [
                "dialogs/qpagesetupdialog_mac.mm",
            ]
        }

        Group {
            name: "Generic Unix sources"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
            files: [
                "dialogs/qpagesetupdialog_unix.cpp",
                "dialogs/qprintdialog_unix.cpp",
                "dialogs/qprintpropertieswidget.ui",
                "dialogs/qprintsettingsoutput.ui",
                "dialogs/qprintwidget.ui",
            ]
        }
    }
}


// MODULE_PLUGIN_TYPES = printsupport
// QMAKE_DOCS = $$PWD/doc/qtprintsupport.qdocconf
// QMAKE_LIBS += $$QMAKE_LIBS_PRINTSUPPORT

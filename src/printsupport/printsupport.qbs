import qbs
import QtPrintsupportConfig
import QtPrintsupportPrivateConfig

QtModuleProject {
    name: "QtPrintSupport"
    simpleName: "printsupport"

    QtHeaders {
        shadowBuildFiles:[
            project.qtbaseShadowDir + "/src/printsupport/qtprintsupport-config.h",
            project.qtbaseShadowDir + "/src/printsupport/qtprintsupport-config_p.h",
        ]
        Depends { name: "QtCoreHeaders" }
    }

    QtPrivateModule {
        Export {
            property var config: QtPrintsupportPrivateConfig
        }
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
            property var config: QtPrintsupportConfig
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }
        Depends { name: "Qt.gui_private" }
        Depends { name: "Qt.widgets_private" }

        cpp.enableExceptions: true
        cpp.includePaths: {
            var result = base.concat(project.includePaths);
            result.push(".");
            if (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin"))
                result.push(project.qtbaseDir + "/src/plugins/printsupport/cups");
            return result;
        }
        cpp.defines: base.concat([
            "QT_NO_FOREACH",
            "QT_NO_USING_NAMESPACE",
        ])
        Properties {
            condition: qbs.targetOS.contains("darwin")
            cpp.frameworks: ["AppKit"]
        }

        Group {
            prefix: "kernel/"
            files: [
                "qpaintengine_alpha.cpp",
                "qpaintengine_alpha_p.h",
                "qplatformprintdevice.cpp",
                "qplatformprintdevice.h",
                "qplatformprintersupport.cpp",
                "qplatformprintersupport.h",
                "qplatformprintplugin.cpp",
                "qplatformprintplugin.h",
                "qprintdevice.cpp",
                "qprintdevice_p.h",
                "qprintengine.h",
                "qprintengine_pdf.cpp",
                "qprinter.cpp",
                "qprinter.h",
                "qprinterinfo.cpp",
                "qprinterinfo.h",
                "qprinterinfo_p.h",
                "qprinter_p.h",
                "qprint_p.h",
                "qtprintsupportglobal.h",
                "qtprintsupportglobal_p.h",
            ]
            Group {
                condition: QtPrintsupportConfig.printpreviewwidget
                files: [
                    "qpaintengine_preview.cpp",
                    "qpaintengine_preview_p.h",
                ]
            }
            Group {
                condition: qbs.targetOS.contains("unix") && QtPrintsupportPrivateConfig.cups
                files: [
                    "qcups.cpp",
                    "qcups_p.h",
                ]
            }
            Group {
                condition: qbs.targetOS.contains("windows")
                files: [
                    "qprintengine_win.cpp",
                    "qprintengine_win_p.h",
                ]
            }
        }
        Properties {
            condition: qbs.targetOS.contains("windows") && !product.targetsUWP
            cpp.dynamicLibraries: ["winspool", "comdlg32", "gdi32", "user32"]
        }
        Group {
            prefix: "widgets/"
            Group {
                condition: QtPrintsupportConfig.printpreviewwidget
                files: [
                    "qprintpreviewwidget.cpp",
                    "qprintpreviewwidget.h",
                ]
            }
            Group {
                condition: QtPrintsupportPrivateConfig.cupsjobwidget
                files: [
                    "qcupsjobwidget.cpp",
                    "qcupsjobwidget.ui",
                    "qcupsjobwidget_p.h",
                ]
            }
        }
        Group {
            prefix: "dialogs/"
            Group {
                condition: QtPrintsupportConfig.printdialog
                files: [
                    "qabstractprintdialog.cpp",
                    "qabstractprintdialog.h",
                    "qabstractprintdialog_p.h",
                    "qpagesetupdialog.cpp",
                    "qpagesetupdialog.h",
                    "qpagesetupdialog_p.h",
                    "qpagesetupwidget.ui",
                    "qprintdialog.h",
                    "qprintdialog.qrc",
                ]
                Group {
                    condition: qbs.targetOS.contains("macos")
                    files: [
                        "qpagesetupdialog_mac.mm",
                        "qprintdialog_mac.mm",
                    ]
                }
                Group {
                    condition: qbs.targetOS.contains("windows")
                    files: [
                        "qpagesetupdialog_win.cpp",
                        "qprintdialog_win.cpp",
                    ]
                }
                Group {
                    condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
                    files: [
                        "qpagesetupdialog_unix_p.h",
                        "qpagesetupdialog_unix.cpp",
                        "qprintdialog_unix.cpp",
                        "qprintpropertieswidget.ui",
                        "qprintsettingsoutput.ui",
                        "qprintwidget.ui",
                    ]
                }
                Group {
                    condition: QtPrintsupportConfig.printpreviewdialog
                    files: [
                        "qprintpreviewdialog.cpp",
                        "qprintpreviewdialog.h",
                    ]
                }
            }
        }
    }
}

/*
MODULE_PLUGIN_TYPES = \
    printsupport
*/

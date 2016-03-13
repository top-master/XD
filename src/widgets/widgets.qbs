import qbs
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtWidgets"
    simpleName: "widgets"
    prefix: project.qtbasePrefix + "src/widgets/"

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
        Depends { name: "QtCoreHeaders" }
        Depends { name: "QtGuiHeaders" }
        ModuleHeaders { fileTags: "hpp_syncable" }
    }

    QtModule {
        name: root.moduleName
        condition: project.widgets
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends { name: "gl"; condition: project.opengl && qbs.targetOS.contains("unix") && !(qbs.targetOS.contains("darwin") && !qbs.targetOS.contains("osx")) }
        Depends { name: "glesv2"; condition: project.opengles2 }
        Depends { name: "gtk+"; condition: project.gtkstyle }
        Depends { name: "x11"; condition: project.gtkstyle }
        Depends { name: "uic"; profiles: project.hostProfile }
        Depends {
            name: "Qt"
            submodules: ["core", "core-private", "gui", "gui-private", "platformheaders"]
        }

        cpp.defines: {
            var defines = base;
            defines.push("QT_BUILD_WIDGETS_LIB");
            if (!project.macstyle)
                defines.push("QT_NO_STYLE_MAC");
            if (!project.windowsstyle)
                defines.push("QT_NO_STYLE_WINDOWS");
            if (!project.windowscestyle)
                defines.push("QT_NO_STYLE_WINDOWSCE");
            if (!project.windowsmobilestyle)
                defines.push("QT_NO_STYLE_WINDOWSMOBILE");
            if (!project.windowsxpstyle)
                defines.push("QT_NO_STYLE_WINDOWSXP");
            if (!project.windowsvistastyle)
                defines.push("QT_NO_STYLE_WINDOWSVISTA");
            if (!project.gtkstyle)
                defines.push("QT_NO_STYLE_GTK");
            if (!project.fusionstyle)
                defines.push("QT_NO_STYLE_FUSION");
            if (!project.androidstyle)
                defines.push("QT_NO_STYLE_ANDROID");
            return defines;
        }

        cpp.dynamicLibraries: {
            var dynamicLibraries = base;
            if (qbs.targetOS.contains("windows")) {
                dynamicLibraries.push("gdi32");
                dynamicLibraries.push("shell32");
                dynamicLibraries.push("user32");
            }
            if (qbs.targetOS.contains("osx")) {
                dynamicLibraries.push("objc");
            }
            return dynamicLibraries;
        }

        cpp.includePaths: root.includePaths.concat(base)

        ModuleHeaders {
            excludeFiles: {
                var excludeFiles = ["doc/**"];
                if (!qbs.targetOS.contains("osx")) {
                    excludeFiles.push("widgets/qmaccocoaviewcontainer_mac.h");
                    excludeFiles.push("widgets/qmacnativewidget_mac.h");
                }
                if (!project.macstyle) {
                    excludeFiles.push("styles/qmacstyle_mac_p.h");
                    excludeFiles.push("styles/qmacstyle_mac_p_p.h");
                }
                if (!project.windowsvistastyle) {
                    excludeFiles.push("styles/qwindowsvistastyle_p_p.h");
                }
                return excludeFiles;
            }
        }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "accessible/complexwidgets.cpp",
                "accessible/itemviews.cpp",
                "accessible/qaccessiblemenu.cpp",
                "accessible/qaccessiblewidget.cpp",
                "accessible/qaccessiblewidgetfactory.cpp",
                "accessible/qaccessiblewidgets.cpp",
                "accessible/rangecontrols.cpp",
                "accessible/simplewidgets.cpp",
                "dialogs/qcolordialog.cpp",
                "dialogs/qdialog.cpp",
                "dialogs/qerrormessage.cpp",
                "dialogs/qfiledialog.cpp",
                "dialogs/qfileinfogatherer.cpp",
                "dialogs/qfilesystemmodel.cpp",
                "dialogs/qfontdialog.cpp",
                "dialogs/qinputdialog.cpp",
                "dialogs/qmessagebox.cpp",
                "dialogs/qprogressdialog.cpp",
                "dialogs/qsidebar.cpp",
                "dialogs/qwizard.cpp",
                "dialogs/qmessagebox.qrc",
                "dialogs/qfiledialog.ui",
                "dialogs/qfiledialog_embedded.ui",
                "effects/qgraphicseffect.cpp",
                "effects/qpixmapfilter.cpp",
                "graphicsview/qgraphicsanchorlayout.cpp",
                "graphicsview/qgraphicsanchorlayout_p.cpp",
                "graphicsview/qgraphicsgridlayout.cpp",
                "graphicsview/qgraphicsgridlayoutengine.cpp",
                "graphicsview/qgraphicsitem.cpp",
                "graphicsview/qgraphicsitemanimation.cpp",
                "graphicsview/qgraphicslayout.cpp",
                "graphicsview/qgraphicslayoutitem.cpp",
                "graphicsview/qgraphicslayout_p.cpp",
                "graphicsview/qgraphicslayoutstyleinfo.cpp",
                "graphicsview/qgraphicslinearlayout.cpp",
                "graphicsview/qgraphicsproxywidget.cpp",
                "graphicsview/qgraphicsscene.cpp",
                "graphicsview/qgraphicsscene_bsp.cpp",
                "graphicsview/qgraphicsscenebsptreeindex.cpp",
                "graphicsview/qgraphicssceneevent.cpp",
                "graphicsview/qgraphicssceneindex.cpp",
                "graphicsview/qgraphicsscenelinearindex.cpp",
                "graphicsview/qgraphicstransform.cpp",
                "graphicsview/qgraphicsview.cpp",
                "graphicsview/qgraphicswidget.cpp",
                "graphicsview/qgraphicswidget_p.cpp",
                "graphicsview/qsimplex_p.cpp",
                "itemviews/qabstractitemdelegate.cpp",
                "itemviews/qabstractitemview.cpp",
                "itemviews/qbsptree.cpp",
                "itemviews/qcolumnview.cpp",
                "itemviews/qcolumnviewgrip.cpp",
                "itemviews/qdatawidgetmapper.cpp",
                "itemviews/qdirmodel.cpp",
                "itemviews/qfileiconprovider.cpp",
                "itemviews/qheaderview.cpp",
                "itemviews/qitemdelegate.cpp",
                "itemviews/qitemeditorfactory.cpp",
                "itemviews/qlistview.cpp",
                "itemviews/qlistwidget.cpp",
                "itemviews/qstyleditemdelegate.cpp",
                "itemviews/qtableview.cpp",
                "itemviews/qtablewidget.cpp",
                "itemviews/qtreeview.cpp",
                "itemviews/qtreewidget.cpp",
                "itemviews/qtreewidgetitemiterator.cpp",
                "kernel/qaction.cpp",
                "kernel/qactiongroup.cpp",
                "kernel/qapplication.cpp",
                "kernel/qboxlayout.cpp",
                "kernel/qdesktopwidget.cpp",
                "kernel/qformlayout.cpp",
                "kernel/qgesture.cpp",
                "kernel/qgesturemanager.cpp",
                "kernel/qgesturerecognizer.cpp",
                "kernel/qgridlayout.cpp",
                "kernel/qlayout.cpp",
                "kernel/qlayoutengine.cpp",
                "kernel/qlayoutitem.cpp",
                "kernel/qmacgesturerecognizer.cpp",
                "kernel/qopenglwidget.cpp",
                "kernel/qshortcut.cpp",
                "kernel/qsizepolicy.cpp",
                "kernel/qstackedlayout.cpp",
                "kernel/qstandardgestures.cpp",
                "kernel/qtooltip.cpp",
                "kernel/qwhatsthis.cpp",
                "kernel/qwidget.cpp",
                "kernel/qwidgetaction.cpp",
                "kernel/qwidgetbackingstore.cpp",
                "kernel/qwidgetsvariant.cpp",
                "kernel/qwidgetwindow.cpp",
                "kernel/qwindowcontainer.cpp",
                "statemachine/qbasickeyeventtransition.cpp",
                "statemachine/qbasicmouseeventtransition.cpp",
                "statemachine/qguistatemachine.cpp",
                "statemachine/qkeyeventtransition.cpp",
                "statemachine/qmouseeventtransition.cpp",
                "styles/qcommonstyle.cpp",
                "styles/qdrawutil.cpp",
                "styles/qfusionstyle.cpp",
                "styles/qpixmapstyle.cpp",
                "styles/qproxystyle.cpp",
                "styles/qstyle.cpp",
                "styles/qstyleanimation.cpp",
                "styles/qstylefactory.cpp",
                "styles/qstylehelper.cpp",
                "styles/qstyleoption.cpp",
                "styles/qstylepainter.cpp",
                "styles/qstyleplugin.cpp",
                "styles/qstylesheetstyle.cpp",
                "styles/qstylesheetstyle_default.cpp",
                "styles/qstyle.qrc",
                "util/qcolormap.cpp",
                "util/qcompleter.cpp",
                "util/qflickgesture.cpp",
                "util/qscroller.cpp",
                "util/qscrollerproperties.cpp",
                "util/qsystemtrayicon.cpp",
                "util/qundogroup.cpp",
                "util/qundostack.cpp",
                "util/qundoview.cpp",
                "widgets/qabstractbutton.cpp",
                "widgets/qabstractscrollarea.cpp",
                "widgets/qabstractslider.cpp",
                "widgets/qabstractspinbox.cpp",
                "widgets/qbuttongroup.cpp",
                "widgets/qcalendarwidget.cpp",
                "widgets/qcheckbox.cpp",
                "widgets/qcombobox.cpp",
                "widgets/qcommandlinkbutton.cpp",
                "widgets/qdatetimeedit.cpp",
                "widgets/qdial.cpp",
                "widgets/qdialogbuttonbox.cpp",
                "widgets/qdockarealayout.cpp",
                "widgets/qdockwidget.cpp",
                "widgets/qeffects.cpp",
                "widgets/qfocusframe.cpp",
                "widgets/qfontcombobox.cpp",
                "widgets/qframe.cpp",
                "widgets/qgroupbox.cpp",
                "widgets/qkeysequenceedit.cpp",
                "widgets/qlabel.cpp",
                "widgets/qlcdnumber.cpp",
                "widgets/qlineedit.cpp",
                "widgets/qlineedit_p.cpp",
                "widgets/qmainwindow.cpp",
                "widgets/qmainwindowlayout.cpp",
                "widgets/qmdiarea.cpp",
                "widgets/qmdisubwindow.cpp",
                "widgets/qmenu.cpp",
                "widgets/qmenubar.cpp",
                "widgets/qplaintextedit.cpp",
                "widgets/qprogressbar.cpp",
                "widgets/qpushbutton.cpp",
                "widgets/qradiobutton.cpp",
                "widgets/qrubberband.cpp",
                "widgets/qscrollarea.cpp",
                "widgets/qscrollbar.cpp",
                "widgets/qsizegrip.cpp",
                "widgets/qslider.cpp",
                "widgets/qspinbox.cpp",
                "widgets/qsplashscreen.cpp",
                "widgets/qsplitter.cpp",
                "widgets/qstackedwidget.cpp",
                "widgets/qstatusbar.cpp",
                "widgets/qtabbar.cpp",
                "widgets/qtabwidget.cpp",
                "widgets/qtextbrowser.cpp",
                "widgets/qtextedit.cpp",
                "widgets/qtoolbar.cpp",
                "widgets/qtoolbararealayout.cpp",
                "widgets/qtoolbarextension.cpp",
                "widgets/qtoolbarlayout.cpp",
                "widgets/qtoolbarseparator.cpp",
                "widgets/qtoolbox.cpp",
                "widgets/qtoolbutton.cpp",
                "widgets/qwidgetanimator.cpp",
                "widgets/qwidgetlinecontrol.cpp",
                "widgets/qwidgetresizehandler.cpp",
                "widgets/qwidgettextcontrol.cpp",
            ]
        }

        Group {
            name: "sources_windows"
            condition: qbs.targetOS.contains("windows")
            prefix: root.prefix
            files: [
                "dialogs/qwizard_win.cpp",
                "util/qsystemtrayicon_win.cpp",
            ]
        }

        Group {
            name: "sources_osx"
            condition: qbs.targetOS.contains("osx")
            prefix: root.prefix
            files: [
                "widgets/qmaccocoaviewcontainer_mac.mm",
                "widgets/qmacnativewidget_mac.mm",
                "widgets/qmenu_mac.mm"
            ]
        }

        Group {
            name: "sources_wince"
            condition: qbs.targetOS.contains("wince")
            prefix: root.prefix
            files: [
                "kernel/qwidgetsfunctions_wince.cpp",
                "util/qsystemtrayicon_wince.cpp",
                "widgets/qmenu_wince.cpp",
            ]
        }

        Group {
            name: "sources_xcb"
            condition: !qbs.targetOS.contains("windows") && project.xcb
            prefix: root.prefix
            files: [
                "util/qsystemtrayicon_x11.cpp",
            ]
        }

        Group {
            name: "sources_qpa"
            condition: !qbs.targetOS.contains("windows") && !project.xcb
            prefix: root.prefix
            files: [
                "util/qsystemtrayicon_qpa.cpp",
            ]
        }

        Group {
            name: "sources_androidstyle"
            condition: project.androidstyle
            prefix: root.prefix + "styles/"
            files: [
                "qandroidstyle.cpp",
            ]
        }

        Group {
            name: "sources_macstyle"
            condition: project.macstyle
            prefix: root.prefix + "styles/"
            files: [
                "qmacstyle_mac.mm"
            ]
        }

        Group {
            name: "sources_windowsstyle"
            condition: project.windowsstyle
            prefix: root.prefix + "styles/"
            files: [
                "qwindowsstyle.cpp",
            ]
        }

        Group {
            name: "sources_windowscestyle"
            condition: project.windowscestyle
            prefix: root.prefix + "styles/"
            files: [
                "qwindowscestyle.cpp",
            ]
        }

        Group {
            name: "sources_windowsmobilestyle"
            condition: project.windowsmobilestyle
            prefix: root.prefix + "styles/"
            files: [
                "qwindowsmobilestyle.cpp",
            ]
        }

        Group {
            name: "sources_windowsxpstyle"
            condition: project.windowsxpstyle
            prefix: root.prefix + "styles/"
            files: [
                "qwindowsxpstyle.cpp",
            ]
        }

        Group {
            name: "sources_windowsvistastyle"
            condition: project.windowsvistastyle
            prefix: root.prefix + "styles/"
            files: [
                "qwindowsvistastyle.cpp",
            ]
        }
    }
}

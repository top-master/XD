Group {
    prefix: path + "/"
    files: [
        "qabstractscrollarea.cpp",
        "qabstractscrollarea.h",
        "qabstractscrollarea_p.h",
        "qabstractspinbox.cpp",
        "qabstractspinbox.h",
        "qabstractspinbox_p.h",
        "qbuttongroup.cpp",
        "qbuttongroup.h",
        "qbuttongroup_p.h",
        "qcalendarwidget.cpp",
        "qcalendarwidget.h",
        "qcombobox.cpp",
        "qcombobox.h",
        "qcombobox_p.h",
        "qdatetimeedit.cpp",
        "qdatetimeedit.h",
        "qdatetimeedit_p.h",
        "qdial.cpp",
        "qdial.h",
        "qdockarealayout.cpp",
        "qdockarealayout_p.h",
        "qdockwidget.cpp",
        "qdockwidget.h",
        "qdockwidget_p.h",
        "qeffects.cpp",
        "qfocusframe.cpp",
        "qfocusframe.h",
        "qfontcombobox.cpp",
        "qfontcombobox.h",
        "qframe.cpp",
        "qframe.h",
        "qframe_p.h",
        "qgroupbox.cpp",
        "qgroupbox.h",
        "qkeysequenceedit.cpp",
        "qkeysequenceedit.h",
        "qkeysequenceedit_p.h",
        "qlineedit.cpp",
        "qlineedit.h",
        "qlineedit_p.cpp",
        "qlineedit_p.h",
        "qmainwindow.cpp",
        "qmainwindow.h",
        "qmainwindowlayout.cpp",
        "qmainwindowlayout_p.h",
        "qmdiarea.cpp",
        "qmdiarea.h",
        "qmdiarea_p.h",
        "qmdisubwindow.cpp",
        "qmdisubwindow.h",
        "qmdisubwindow_p.h",
        "qmenu.cpp",
        "qmenu.h",
        "qmenu_p.h",
        "qmenubar.cpp",
        "qmenubar.h",
        "qmenubar_p.h",
        "qplaintextedit.cpp",
        "qplaintextedit.h",
        "qplaintextedit_p.h",
        "qprogressbar.cpp",
        "qprogressbar.h",
        "qrubberband.cpp",
        "qrubberband.h",
        "qscrollarea.cpp",
        "qscrollarea.h",
        "qscrollarea_p.h",
        "qscrollbar.cpp",
        "qscrollbar.h",
        "qscrollbar_p.h",
        "qsizegrip.cpp",
        "qsizegrip.h",
        "qslider.cpp",
        "qslider.h",
        "qspinbox.cpp",
        "qspinbox.h",
        "qsplitter.cpp",
        "qsplitter.h",
        "qsplitter_p.h",
        "qstackedwidget.cpp",
        "qstackedwidget.h",
        "qstatusbar.cpp",
        "qstatusbar.h",
        "qtabbar.cpp",
        "qtabbar.h",
        "qtabbar_p.h",
        "qtabwidget.cpp",
        "qtabwidget.h",
        "qtextbrowser.cpp",
        "qtextbrowser.h",
        "qtextedit.cpp",
        "qtextedit.h",
        "qtextedit_p.h",
        "qtoolbarextension.cpp",
        "qtoolbarextension_p.h",
        "qtoolbox.cpp",
        "qtoolbox.h",
        "qtoolbutton.cpp",
        "qtoolbutton.h",
        "qwidgetanimator.cpp",
        "qwidgetanimator_p.h",
        "qwidgetlinecontrol.cpp",
        "qwidgetlinecontrol_p.h",
        "qwidgetresizehandler.cpp",
        "qwidgetresizehandler_p.h",
    ]
    Group {
        condition: product.config.abstractbutton
        files: [
            "qabstractbutton.cpp",
            "qabstractbutton.h",
            "qabstractbutton_p.h",
        ]
    }
    Group {
        condition: product.config.abstractslider
        files: [
            "qabstractslider.cpp",
            "qabstractslider.h",
            "qabstractslider_p.h",
        ]
    }
    Group {
        condition: product.config.checkbox
        files: [
            "qcheckbox.cpp",
            "qcheckbox.h",
        ]
    }
    Group {
        condition: product.config.commandlinkbutton
        files: [
            "qcommandlinkbutton.cpp",
            "qcommandlinkbutton.h",
        ]
    }
    Group {
        condition: product.config.label
        files: [
            "qlabel.cpp",
            "qlabel.h",
            "qlabel_p.h",
        ]
    }
    Group {
        condition: product.config.lcdnumber
        files: [
            "qlcdnumber.cpp",
            "qlcdnumber.h",
        ]
    }
    Group {
        condition: product.config.pushbutton
        files: [
            "qpushbutton.cpp",
            "qpushbutton.h",
            "qpushbutton_p.h",
        ]
    }
    Group {
        condition: product.config.radiobutton
        files: [
            "qradiobutton.cpp",
            "qradiobutton.h",
        ]
    }
    Group {
        condition: product.config.dialogbuttonbox
        files: [
            "qdialogbuttonbox.cpp",
            "qdialogbuttonbox.h",
        ]
    }
    Group {
        condition: product.config.splashscreen
        files: [
            "qsplashscreen.cpp",
            "qsplashscreen.h",
        ]
    }
    Group {
        condition: product.privateConfig.widgettextcontrol
        files: [
            "qwidgettextcontrol.cpp",
            "qwidgettextcontrol_p.h",
            "qwidgettextcontrol_p_p.h",
        ]
    }
    Group {
        condition: product.config.toolbar
        files: [
            "qtoolbar.cpp",
            "qtoolbar.h",
            "qtoolbar_p.h",
            "qtoolbararealayout.cpp",
            "qtoolbararealayout_p.h",
            "qtoolbarlayout.cpp",
            "qtoolbarlayout_p.h",
            "qtoolbarseparator.cpp",
            "qtoolbarseparator_p.h",
        ]
    }

    Group {
        condition: qbs.targetOS.contains("macos")
        files: [
            "qmaccocoaviewcontainer_mac.h",
            "qmaccocoaviewcontainer_mac.mm",
            "qmacnativewidget_mac.h",
            "qmacnativewidget_mac.mm",
            "qmenu_mac.mm",
        ]
    }
}

Group {
    prefix: path + "/"
    files: [
        "qaction.cpp",
        "qaction.h",
        "qaction_p.h",
        "qactiongroup.cpp",
        "qactiongroup.h",
        "qapplication.cpp",
        "qapplication.h",
        "qapplication_p.h",
        "qboxlayout.cpp",
        "qboxlayout.h",
        "qdesktopwidget.cpp",
        "qdesktopwidget.h",
        "qdesktopwidget_p.h",
        "qgesture.cpp",
        "qgesture.h",
        "qgesture_p.h",
        "qgesturemanager.cpp",
        "qgesturemanager_p.h",
        "qgesturerecognizer.cpp",
        "qgesturerecognizer.h",
        "qgridlayout.cpp",
        "qgridlayout.h",
        "qlayout.cpp",
        "qlayout.h",
        "qlayout_p.h",
        "qlayoutengine.cpp",
        "qlayoutengine_p.h",
        "qlayoutitem.cpp",
        "qlayoutitem.h",
        "qshortcut.cpp",
        "qshortcut.h",
        "qsizepolicy.cpp",
        "qsizepolicy.h",
        "qstackedlayout.cpp",
        "qstackedlayout.h",
        "qstandardgestures.cpp",
        "qstandardgestures_p.h",
        "qtooltip.cpp",
        "qtooltip.h",
        "qtwidgetsglobal.h",
        "qtwidgetsglobal_p.h",
        "qwhatsthis.cpp",
        "qwhatsthis.h",
        "qwidget.cpp",
        "qwidget.h",
        "qwidget_p.h",
        "qwidgetaction.cpp",
        "qwidgetaction.h",
        "qwidgetaction_p.h",
        "qwidgetbackingstore.cpp",
        "qwidgetbackingstore_p.h",
        "qwidgetsvariant.cpp",
        "qwidgetwindow.cpp",
        "qwidgetwindow_p.h",
        "qwindowcontainer.cpp",
        "qwindowcontainer_p.h",
    ]
    Group {
        files: [
            "qt_widgets_pch.h",
        ]
        fileTags: ["cpp_pch_src"]
    }
    Group {
        condition: qbs.targetOS.contains("macos")
        files: [
            "qmacgesturerecognizer.cpp",
            "qmacgesturerecognizer_p.h",
        ]
    }
    Group {
        condition: Qt.gui.config.opengl
        files: [
            "qopenglwidget.cpp",
            "qopenglwidget.h",
        ]
    }
    Group {
        condition: product.config.formlayout
        files: [
            "qformlayout.cpp",
            "qformlayout.h",
        ]
    }
}

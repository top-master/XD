Group {
    prefix: path + "/"
    files: [
        "qcolordialog.cpp",
        "qcolordialog.h",
        "qerrormessage.cpp",
        "qerrormessage.h",
        "qfiledialog.cpp",
        "qfiledialog.h",
        "qfiledialog.ui",
        "qfiledialog_p.h",
        "qfileinfogatherer.cpp",
        "qfileinfogatherer_p.h",
        "qfilesystemmodel.cpp",
        "qfilesystemmodel.h",
        "qfilesystemmodel_p.h",
        "qfontdialog.cpp",
        "qfontdialog.h",
        "qfontdialog_p.h",
        "qfscompleter_p.h",
        "qinputdialog.cpp",
        "qinputdialog.h",
        "qmessagebox.cpp",
        "qmessagebox.h",
        "qmessagebox.qrc",
        "qprogressdialog.cpp",
        "qprogressdialog.h",
        "qsidebar.cpp",
        "qsidebar_p.h",
        "qwizard.cpp",
        "qwizard.h",
    ]
    Group {
        condition: qbs.targetOS.contains("windows")
        files: [
            "qwizard_win.cpp",
            "qwizard_win_p.h",
        ]
    }
    Group {
        condition: product.config.dialog
        files: [
            "qdialog.cpp",
            "qdialog.h",
            "qdialog_p.h",
        ]
    }
}

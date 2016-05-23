import qbs

// TODO: We'd like to have more than one group here. Can we turn this into a module?
// Or do we need QBS-59?
Group {
    name: "headers"
    files: {
        var files = [
            "dialogs/qabstractprintdialog.h",
            "dialogs/qabstractprintdialog_p.h",
            "dialogs/qpagesetupdialog_p.h",
            "dialogs/qpagesetupdialog.h",
            "dialogs/qprintdialog.h",
            "dialogs/qprintpreviewdialog.h",
            "kernel/qpaintengine_alpha_p.h",
            "kernel/qpaintengine_preview_p.h",
            "kernel/qplatformprintdevice.h",
            "kernel/qplatformprintersupport.h",
            "kernel/qplatformprintplugin.h",
            "kernel/qprint_p.h",
            "kernel/qprintdevice_p.h",
            "kernel/qprintengine.h",
            "kernel/qprintengine_pdf_p.h",
            "kernel/qprinter.h",
            "kernel/qprinter_p.h",
            "kernel/qprinterinfo.h",
            "kernel/qprinterinfo_p.h",
            "kernel/qtprintsupportglobal.h",
            "widgets/qprintpreviewwidget.h",
        ];
        if (qbs.targetOS.contains("windows"))
            files.push("kernel/qprintengine_win_p.h");
        if (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin") && project.cups) {
            files.push("kernel/qcups_p.h");
            files.push("widgets/qcupsjobwidget_p.h");
        }
        if (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin"))
            files.push("dialogs/qpagesetupdialog_unix_p.h");
        return files;
    }
    overrideTags: true
}

import qbs

QtExample {
    name: "sqlbrowser"
    condition: Qt.sql.present && Qt.widgets.present && profile == project.hostProfile

    Depends { name: "Qt.sql"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    cpp.includePaths: base.concat('.')

    files: [
        "browser.cpp",
        "browser.h",
        "browserwidget.ui",
        "connectionwidget.cpp",
        "connectionwidget.h",
        "main.cpp",
        "qsqlconnectiondialog.cpp",
        "qsqlconnectiondialog.h",
        "qsqlconnectiondialog.ui",
    ]
}

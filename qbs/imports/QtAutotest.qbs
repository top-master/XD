import qbs

QtProduct {
    condition: project.config.contains("testcase_targets")
    type: ["application", "autotest"]
    consoleApplication: true
    cpp.defines: base.filter(function(define) {
        return define != "QT_ASCII_CAST_WARNINGS" && define != "QT_USE_QSTRINGBUILDER"
            && define != "QT_DEPRECATED_WARNINGS";
    })
    Depends { name: "Qt.core" }
    Depends { name: "Qt.testlib" }
}

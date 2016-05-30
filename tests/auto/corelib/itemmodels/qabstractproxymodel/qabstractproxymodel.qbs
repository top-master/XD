import qbs

QtAutotest {
    name: "tst_qabstractproxymodel"
    condition: project.gui
    Depends { name: "Qt.gui"; condition: project.gui }
    cpp.defines: base.concat(["QT_DISABLE_DEPRECATED_BEFORE=0"])
    files: [
        "tst_qabstractproxymodel.cpp",
    ]
}

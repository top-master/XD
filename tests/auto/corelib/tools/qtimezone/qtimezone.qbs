import qbs

QtAutotest {
    name: "tst_qtimezone"

    Depends { name: "Qt.core-private" }

    Properties {
        condition: project.icu
        cpp.defines: base.concat(["QT_USE_ICU"])
    }

    files: [
        "tst_qtimezone.cpp",
    ]
}

import qbs

QtAutotest {
    name: "tst_qdatetime"

    Depends { name: "Qt.core-private" }

    Properties {
        condition: qbs.targetOS.contains("darwin")
        cpp.frameworks: base.concat(["Foundation"])
    }

    files: [
        "tst_qdatetime.cpp",
    ]

    Group {
        name: "mac sources"
        condition: qbs.targetOS.contains("darwin")
        files: [
            "tst_qdatetime_mac.mm",
        ]
    }
}

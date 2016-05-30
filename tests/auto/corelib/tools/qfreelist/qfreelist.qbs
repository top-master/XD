import qbs

QtAutotest {
    name: "tst_qfreelist"

    Depends { name: "Qt.core-private" }

    files: [
        "tst_qfreelist.cpp",
    ]

    Group {
        name: "sources from QtCore"
        condition: !project.config.contains("private_tests")
        files: [
            project.qtbasePrefix + "src/corelib/tools/qfreelist.cpp",
        ]
    }
}

import qbs

QtAutotest {
    name: "tst_qmimedatabase-cache"
    condition: base && Qt.global.config.concurrent && Qt.global.privateConfig.private_tests
               && qbs.targetOS.contains("unix") && !qbs.targetOS.contains("qnx")
               && !qbs.targetOS.contains("darwin")

    Depends { name: "Qt.concurrent" }

    cpp.cxxFlags: base.concat("-Wshadow", "-Wno-long-long",  "-Wnon-virtual-dtor")
    cpp.defines: base.concat("USE_XDG_DATA_DIRS")

    files: [
        project.qtbaseDir + "/src/corelib/mimetypes/mimetypes.qrc",
        "../testdata.qrc",
        "../tst_qmimedatabase.h",
        "tst_qmimedatabase-cache.cpp",
    ]
}

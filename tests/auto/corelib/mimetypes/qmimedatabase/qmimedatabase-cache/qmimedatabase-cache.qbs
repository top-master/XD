import qbs

QtAutotest {
    name: "tst_qmimedatabase-cache"
    condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("qnx")
               && !qbs.targetOS.contains("darwin") && project.concurrent

    Depends { name: "Qt.concurrent"; condition: project.concurrent }

    cpp.cxxFlags: base.concat(["-Wshadow", "-Wno-long-long",  "-Wnon-virtual-dtor"])
    cpp.defines: base.concat(["USE_XDG_DATA_DIRS"])

    files: [
        project.qtbasePrefix + "/src/corelib/mimetypes/mimetypes.qrc",
        "../testdata.qrc",
        "../tst_qmimedatabase.h",
        "tst_qmimedatabase-cache.cpp",
    ]
}

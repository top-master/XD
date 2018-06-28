import qbs

QtAutotest {
    name: "tst_qmimedatabase-xml"
    condition: base && Qt.global.config.concurrent && Qt.global.privateConfig.private_tests

    Depends { name: "Qt.concurrent" }

    Properties {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
                   && !qbs.targetOS.contains("qnx")
        cpp.defines: base.concat("USE_XDG_DATA_DIRS")
    }
    Properties {
        condition: qbs.toolchain.contains("gcc")
        cpp.cxxFlags: base.concat("-Wshadow", "-Wno-long-long", "-Wnon-virtual-dtor")
    }

    files: [
        project.qtbaseDir + "/src/corelib/mimetypes/mimetypes.qrc",
        "../testdata.qrc",
        "../tst_qmimedatabase.h",
        "tst_qmimedatabase-xml.cpp",
    ]
}

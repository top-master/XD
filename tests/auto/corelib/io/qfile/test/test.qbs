import qbs

QtAutotest {
    name: "tst_qfile"
    destinationDirectory: project.buildDirectory + "/qfile_test"
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network"; condition: project.network }
    Depends { name: "stdinprocess" }
    Properties {
        condition: !project.network
        cpp.defines: base.concat(["QT_NO_NETWORK"])
    }
    Properties {
        condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")
        cpp.dynamicLibraries: base.concat(["ole32", "uuid"])
    }

    files: [
        "../copy-fallback.qrc",
        "../qfile.qrc",
        "../rename-fallback.qrc",
        "../tst_qfile.cpp",
    ]
}

//TESTDATA += ../dosfile.txt ../noendofline.txt ../testfile.txt \
//            ../testlog.txt ../two.dots.file ../tst_qfile.cpp \
//            ../Makefile ../forCopying.txt ../forRenaming.txt \
//            ../resources/file1.ext1

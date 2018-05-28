import qbs
import qbs.FileInfo

QtAutotest {
    name: "tst_qfile"
    Depends { name: "Qt.global" }
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.network"; condition: Qt.global.privateConfig.network }
    cpp.defines: {
        var defs = base;
        if (!Qt.global.privateConfig.network)
            defs.push("QT_NO_NETWORK");
        return defs;
    }
    cpp.includePaths: base.concat(project.sharedTestDir)
    Properties {
        condition: qbs.targetOS.contains("windows") && !targetsUWP
        cpp.dynamicLibraries: base.concat(["ole32", "uuid"])
    }

    files: [
        "copy-fallback.qrc",
        "qfile.qrc",
        "rename-fallback.qrc",
    ]

    Group {
        name: "shared sources"
        prefix: project.sharedTestDir + '/'
        files: "emulationdetector.h"
    }

    QtTestData {
        files: [
            "dosfile.txt",
            "forCopying.txt",
            "forRenaming.txt",
            "noendofline.txt",
            "testfile.txt",
            "testlog.txt",
            "two.dots.file",
            "resources/file1.ext1",
        ]
    }

    Group {
        name: "main source"
        files: "tst_qfile.cpp"
        fileTags: "qt.testdata"
        overrideTags: false
    }
}

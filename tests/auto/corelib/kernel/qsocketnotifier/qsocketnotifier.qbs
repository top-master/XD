import qbs

QtAutotest {
    name: "tst_qsocketnotifier"
    condition: project.network && project.private_tests
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.network-private" }

    Properties {
        condition: qbs.targetOS.contains("wince")
        cpp.dynamicLibraries: outer.concat("ws2")
    }
    Properties {
        condition: qbs.targetOS.contains("windows")
            && !qbs.targetOS.contains("wince")
        cpp.dynamicLibraries: outer.concat("ws2_32")
    }

    files: "tst_qsocketnotifier.cpp"

    Group {
        id: netgrp
        name: "files from qtnetwork"
        condition: qbs.targetOS.contains("unix") && project.reduce_exports
        prefix: project.qtbasePrefix + "src/network/socket/"
        cpp.includePaths: outer.concat(netgrp.prefix + "../")
        files: [
            "qabstractsocketengine.cpp",
            "qnativesocketengine.cpp",
            "qnativesocketengine_unix.cpp",
        ]
    }
}


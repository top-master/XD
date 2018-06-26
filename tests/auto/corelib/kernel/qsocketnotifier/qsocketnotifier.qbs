import qbs

QtAutotest {
    name: "tst_qsocketnotifier"
    condition: base && Qt.global.privateConfig.network && Qt.global.privateConfig.private_tests
    supportsUiKit: true

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.network_private" }

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: outer.concat("ws2_32")
    }

    files: "tst_qsocketnotifier.cpp"

    Group {
        id: netgrp
        name: "files from qtnetwork"
        condition: qbs.targetOS.contains("unix") && project.reduce_exports
        prefix: path + "/../../../network/socket/"
        cpp.includePaths: outer.concat(prefix + "..")
        files: [
            "qabstractsocketengine.cpp",
            "qnativesocketengine.cpp",
            "qnativesocketengine_unix.cpp",
        ]
    }
}

import qbs
import qbs.FileInfo
import QtMultiplexConfig

QtProduct {
    condition: Qt.global.config.private_tests && (!hasUiKit || supportsUiKit) && customCondition
    type: ["application", "autotest"]

    consoleApplication: true
    aggregate: false
    property bool supportsUiKit: false
    property bool customCondition: true

    Depends { name: "Qt.core" }
    Depends { name: "Qt.testlib" }
    Depends { name: "Qt.global" }

    cpp.defines: []
    cpp.rpaths: cpp.rpathOrigin + '/' + FileInfo.relativePath(destinationDirectory,
                    FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix, "lib"))
    qbs.buildVariants: QtMultiplexConfig.debug_and_release ? ["debug"] : []
}

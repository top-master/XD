import qbs
import qbs.FileInfo
import QtMultiplexConfig

QtProduct {
    condition: Qt.global.config.private_tests
    type: ["application", "autotest-helper"]
    consoleApplication: true
    qbs.buildVariants: QtMultiplexConfig.debug_and_release ? ["debug"] : []
    cpp.rpaths: cpp.rpathOrigin + '/' + FileInfo.relativePath(destinationDirectory,
                    FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix, "lib"))
    Group {
        fileTagsFilter: "application"
        fileTags: "autotest-helper"
    }
}

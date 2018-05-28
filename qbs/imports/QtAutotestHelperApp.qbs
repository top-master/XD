import qbs
import qbs.FileInfo
import QtMultiplexConfig

QtProduct {
    condition: Qt.global.privateConfig.build_parts.contains("tests")
    type: ["application", "autotest-helper"]
    consoleApplication: true
    qbs.buildVariants: QtMultiplexConfig.debug_and_release ? ["debug"] : []
    Properties {
        condition: doInstall
        cpp.rpaths: cpp.rpathOrigin + '/' + FileInfo.relativePath('/' + installDir, "/lib")
    }
    Properties {
        condition: !doInstall
        cpp.rpaths: FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix, "lib")
    }

    property string testName: "tst_" + FileInfo.fileName(FileInfo.path(sourceDirectory))
    property bool doInstall: !(targetsUWP || hasUiKit || qbs.targetOS.contains("android"))
    property string installSuffix: name
    property string installDir: FileInfo.joinPaths("tests", testName, installSuffix)
    Group {
        fileTagsFilter: "application"
        fileTags: "autotest-helper"
        qbs.install: doInstall
        qbs.installDir: installDir
    }
}

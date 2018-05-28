import qbs
import qbs.File
import qbs.FileInfo
import qbs.Utilities
import QtMultiplexConfig

QtProduct {
    condition: Qt.global.privateConfig.build_parts.contains("tests")
                   && (!hasUiKit || supportsUiKit)
    type: ["application", "autotest"]

    consoleApplication: true
    aggregate: false
    property bool supportsUiKit: false
    property bool builtinTestData: targetsUWP || hasUiKit || qbs.targetOS.contains("android")
    property bool doInstall: true
    property string installDir: FileInfo.joinPaths("tests", name)
    property bool insignificant: false
    property bool haveAutotestModule: Utilities.versionCompare(qbs.version, "1.13") > 0

    Depends { name: "Qt.core" }
    Depends { name: "Qt.testlib" }
    Depends { name: "Qt.global" }

    Depends { productTypes: "autotest-helper" }

    Depends { name: "autotest"; condition: haveAutotestModule }
    Properties {
        condition: haveAutotestModule
        autotest.allowFailure: insignificant
    }

    Properties {
        condition: builtinTestData
        cpp.defines: "BUILTIN_TESTDATA"
    }
    cpp.defines: []

    Properties {
        condition: doInstall
        cpp.rpaths: cpp.rpathOrigin + '/' + FileInfo.relativePath('/' + installDir, "/lib")
    }
    Properties {
        condition: !doInstall
        cpp.rpaths: FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix, "lib")
    }
    qbs.buildVariants: QtMultiplexConfig.debug_and_release ? ["debug"] : []

    property string blacklistFile: FileInfo.joinPaths(sourceDirectory, "BLACKLIST")

    QtTestData {
        name: "blacklist file"
        condition: File.exists(blacklistFile)
        files: blacklistFile
    }

    Group {
        fileTagsFilter: "application"
        qbs.install: doInstall
        qbs.installDir: installDir
    }
    Group {
        condition: !builtinTestData
        fileTagsFilter: "qt.testdata"
        qbs.install: true
        qbs.installDir: installDir
        qbs.installSourceBase: sourceDirectory
    }
    Group {
        condition: !builtinTestData
        fileTagsFilter: "qt.generated_testdata"
        qbs.install: true
        qbs.installDir: installDir
    }
    Group {
        condition: builtinTestData
        fileTagsFilter: ["qt.testdata", "qt.generated_testdata"]
        fileTags: "qt.core.resource_data"
    }
}

import qbs
import qbs.FileInfo
import qbs.ModUtils
import QtCoreConfig

QtHostTool {
    description: "Qt User Interface Compiler"
    toolFileTag: "qt.uic-tool"
    condition: Qt.global.privateConfig.gui && Qt.global.privateConfig.widgets
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/corelib/qbs"]

    createQbsModule: true
    useBootstrapLib: base || !QtCoreConfig.commandlineparser || !QtCoreConfig.textcodec
        || !QtCoreConfig.xmlstreamreader || !QtCoreConfig.xmlstreamwriter
    Depends { name: "Qt.bootstrap_private"; condition: useBootstrapLib }
    Depends { name: "Qt.core_private"; condition: !useBootstrapLib }

    cpp.defines: base.concat(["QT_UIC",
                              "QT_UIC_CPP_GENERATOR",
                              "QT_NO_CAST_FROM_ASCII",
                              "QT_NO_FOREACH"])
    cpp.includePaths: base.concat(".", "cpp")

    Export {
        FileTagger {
            patterns: "*.ui"
            fileTags: "uic"
        }

        Rule {
            inputs: "uic"
            explicitlyDependsOnFromDependencies: ["qt.uic-tool"]
            Artifact {
                fileTags: "hpp"
                filePath: FileInfo.joinPaths(input.Qt.core.generatedHeadersDir,
                                             'ui_' + input.completeBaseName + '.h')
            }
            prepare: {
                var uic = explicitlyDependsOn["qt.uic-tool"][0];
                var uicFilePath = uic.qbs.install ? ModUtils.artifactInstalledFilePath(uic)
                                                  : uic.filePath;
                var cmd = new Command(uicFilePath, ["-o", output.filePath, input.filePath]);
                cmd.description = "uic " + input.fileName;
                cmd.highlight = "codegen";
                return cmd;
            }
        }
    }

    files: [
        "customwidgetsinfo.cpp",
        "customwidgetsinfo.h",
        "databaseinfo.cpp",
        "databaseinfo.h",
        "driver.cpp",
        "driver.h",
        "globaldefs.h",
        "main.cpp",
        "option.h",
        "treewalker.cpp",
        "treewalker.h",
        "ui4.cpp",
        "ui4.h",
        "uic.cpp",
        "uic.h",
        "utils.h",
        "validator.cpp",
        "validator.h",
    ]

    Group {
        condition: true
        name: "cpp generator"
        prefix: "cpp/"
        files: [
            "cppwritedeclaration.cpp",
            "cppwritedeclaration.h",
            "cppwriteincludes.cpp",
            "cppwriteincludes.h",
            "cppwriteinitialization.cpp",
            "cppwriteinitialization.h",
        ]
    }
}

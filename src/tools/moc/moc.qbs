import qbs
import "moc.js" as Moc

QtHostTool {
    description: "Qt Meta Object Compiler"
    toolFileTag: "qt.moc-tool"
    createQbsModule: true
    useBootstrapLib: true

    Depends { name: "QtCoreHeaders" }

    cpp.defines: base.concat(["QT_NO_COMPRESS"])

    Group {
        name: "source"
        files: [
            "token.cpp",
            "generator.cpp",
            "main.cpp",
            "moc.cpp",
            "parser.cpp",
            "preprocessor.cpp",
        ]
    }

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: [
            "shell32",
            "ole32",
        ]
    }

    Group {
        files: ["moc.js"]
        qbs.install: true
        qbs.installDir: moduleInstallDir
    }

    Export {
        property stringList extraArguments: []
        Depends { name: "cpp" }
        cpp.includePaths: [importingProduct.buildDirectory + "/qt.headers"]
        Rule {
            name: "QtCoreMocRule"
            inputs: ["objcpp", "cpp", "hpp"]
            explicitlyDependsOnFromDependencies: ["qt.moc-tool"]
            auxiliaryInputs: ["qt_plugin_metadata"]
            excludedInputs: ["unmocable"]
            outputFileTags: ["hpp", "cpp", "moc_cpp", "unmocable"]
            outputArtifacts: {
                if (input.fileTags.contains("unmocable"))
                    return [];
                var mocinfo = QtMocScanner.apply(input);
                if (!mocinfo.hasQObjectMacro)
                    return [];
                var artifact = { fileTags: ["unmocable"] };
                if (input.fileTags.contains("hpp")) {
                    artifact.filePath = product.buildDirectory + "/qt.headers" + "/moc_"
                        + input.completeBaseName + ".cpp";
                } else {
                    artifact.filePath = product.buildDirectory + "/qt.headers" + '/'
                        + input.completeBaseName + ".moc";
                }
                artifact.fileTags.push(mocinfo.mustCompile ? "cpp" : "hpp");
                if (mocinfo.hasPluginMetaDataMacro)
                    artifact.explicitlyDependsOn = ["qt_plugin_metadata"];
                return [artifact];
            }
            prepare: {
                var mocPath = explicitlyDependsOn["qt.moc-tool"][0].filePath;
                var cmd = new Command(mocPath, Moc.args(product, input, output.filePath));
                cmd.description = 'moc ' + input.fileName;
                cmd.highlight = 'codegen';
                return cmd;
            }
        }
    }
}

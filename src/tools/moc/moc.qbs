import qbs
import qbs.FileInfo
import "moc.js" as Moc

QtHostTool {
    toolFileTag: "qt.moc-tool"
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

    Depends { name: "Exporter.qbs" }
    property string moduleInstallDir: FileInfo.joinPaths("lib", "qbs", "modules", name)
    Group {
        fileTagsFilter: ["Exporter.qbs.module"]
        qbs.install: true
        qbs.installDir: moduleInstallDir
        // ### Excercise for the interested reader: Why does the following line yield the weirdest error?
        // qbs.installPrefix: hey_ho_lets_go
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
            explicitlyDependsOn: ["qt.moc-tool"]
            auxiliaryInputs: ["qt_plugin_metadata"]
            excludedAuxiliaryInputs: ["unmocable"]
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

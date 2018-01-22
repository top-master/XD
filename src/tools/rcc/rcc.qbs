import qbs
import qbs.FileInfo

QtHostTool {
    toolFileTag: "qt.rcc-tool"
    useBootstrapLib: true
    cpp.includePaths: [path].concat(base)

    Group {
        name: "source"
        files: [
            "main.cpp",
            "rcc.cpp",
            "rcc.h",
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
    }

    Export {
        Rule {
            inputs: ["rcc"]
            explicitlyDependsOn: ["qt.rcc-tool"]
            Artifact {
                fileTags: "cpp"
                filePath: product.buildDirectory + "/.rcc/" + input.baseName + "_rcc.cpp"
            }
            prepare: {
                var rccPath = explicitlyDependsOn["qt.rcc-tool"][0].filePath;
                var cmd = new Command(rccPath, [
                    input.filePath,
                    "--name", input.baseName,
                    "-o", output.filePath,
                ]);
                cmd.description = "rcc " + input.fileName;
                cmd.highlight = "codegen";
                return cmd;
            }
        }

        FileTagger {
            patterns: "*.qrc"
            fileTags: "rcc"
        }
    }
}

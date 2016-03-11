import qbs
import qbs.FileInfo
import qbs.Process
import qbs.TextFile

QtProduct {
    type: [project.staticBuild ? "staticlibrary" : "dynamiclibrary"].concat(["prl", "pri"])
    version: project.version
    destinationDirectory: project.libDirectory

    property string parentName
    property string simpleName
    property path qmakeProject
    readonly property string qmakeProjectPrefix: ""

    cpp.commonCompilerFlags: base.concat(project.sse2 ? project.sse2Flags: [])

    FileTagger {
        patterns: ["*_p.h"]
        fileTags: ["qt.private_header"]
    }

    Rule {
        condition: !project.staticBuild && qbs.targetOS.contains("linux")
                   && !qbs.targetOS.contains("android")
        inputs: ["qt.private_header"] // TODO: Make edge case of no private headers work.
        Artifact {
            filePath: product.name + ".versionscript"
            fileTags: ["versionscript"]
        }
        multiplex: true
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "Creating " + output.fileName;
            cmd.sourceCode = function() {
                // TODO: File is empty for "internal modules"
                var version = project.version.split('.');
                var replacerScript = project.qtbasePrefix
                        + "mkspecs/features/data/unix/findclasslist.pl";
                var replaceProcess = new Process();
                // TODO: Make the perl path configurable
                if (!replaceProcess.start("perl", [replacerScript]))
                    throw "Failed to generate linker version script";
                replaceProcess.writeLine("Qt_" + version[0]
                                         + "_PRIVATE_API {\n qt_private_api_tag*;");
                for (var i = 0; i < inputs["qt.private_header"].length; ++i) {
                    var headerPath = inputs["qt.private_header"][i].filePath;
                    replaceProcess.writeLine("    @FILE:" + headerPath + "@");
                }
                replaceProcess.writeLine("};");
                var current = "Qt_" + version[0];
                replaceProcess.writeLine(current + " { *; };");
                var versionTag = "qt_version_tag"; // TODO: Append "_" + Qt namespace, if there is one
                for (i = 0; i <= version[1]; ++i) {
                    console.log("(test)");
                    var previous = current
                    current = "Qt_" + version[0] + "." + i;
                    if (i == version[1]) {
                        replaceProcess.writeLine(current + " { " + versionTag  + "; } "
                                                 + previous + ";");
                    } else {
                        replaceProcess.writeLine(current + " {} " + previous + ";");
                    }
                }
                replaceProcess.closeWriteChannel();
                if (!replaceProcess.waitForFinished()) {
                    replaceProcess.kill();
                    throw "Failed to generate linker version script";
                }
                var outFile = new TextFile(output.filePath, TextFile.WriteOnly);
                outFile.write(replaceProcess.readStdOut());
                outFile.close();
                replaceProcess.close();
            };
            return [cmd];
        }
    }

    Group {
        fileTagsFilter: [
            "debuginfo",
            "dynamiclibrary",
            "dynamiclibrary_import",
            "dynamiclibrary_symlink",
            "staticlibrary",
        ]
        qbs.install: true
        qbs.installDir: "lib"
        qbs.installSourceBase: FileInfo.joinPaths(project.buildDirectory, "lib")
    }

    // The following files are for qmake compatibility, so use qmake to generate them.
    Group {
        condition: project.qmake
        name: "qmake_project"
        files: qmakeProject
        fileTags: "module_pro"
    }

    Rule {
        inputs: "module_pro"
        outputArtifacts: {
            var outputArtifacts = [
                {
                    filePath: product.qmakeProjectPrefix + product.parentName + ".version",
                    fileTags: "version",
                },
                {
                    filePath: project.configPath + '/lib/' + product.moduleProperty("cpp", "dynamicLibraryPrefix") + product.targetName + ".prl",
                    fileTags: "prl",
                },
                {
                    filePath: project.configPath + '/mkspecs/modules/qt_lib_' + product.simpleName + ".pri",
                    fileTags: "pri",
                },
            ];
            if (!product.simpleName.endsWith("_private")) {
                outputArtifacts.push({
                    filePath: project.configPath + '/mkspecs/modules/qt_lib_' + product.simpleName + "_private.pri",
                    fileTags: "pri",
                })
            }
            return outputArtifacts;
        }
        outputFileTags: ["version", "prl", "pri"]
        prepare: {
            var cmd = new Command(project.configPath + "/bin/qmake", [
                "-prl", input.filePath,
            ]);
            cmd.workingDirectory = FileInfo.path(outputs.version[0].filePath);
            cmd.description = "generating prl/pri files for " + product.name;
            cmd.highlight = "codegen";
            return cmd;
        }
    }

    Group {
        fileTagsFilter: "pri"
        qbs.install: true
        qbs.installDir: "mkspecs/modules"
    }

    Group {
        fileTagsFilter: "prl"
        qbs.install: true
        qbs.installDir: "lib"
    }
}

import qbs
import qbs.FileInfo
import "QtUtils.js" as QtUtils

Product {
    property bool hostBuild: false
    profiles: project.targetProfiles

    Depends { name: "cpp" }
    Depends { name: "osversions" }
    Depends { name: "rcc"; condition: !product.hostBuild; profiles: project.hostProfile }
    Depends { name: "moc"; condition: !product.hostBuild; profiles: project.hostProfile }
    Depends { name: "Android.ndk"; condition: qbs.targetOS.contains("android") }

    Properties {
        condition: qbs.targetOS.contains("android")
        Android.ndk.appStl: "gnustl_shared"
    }

    cpp.cxxLanguageVersion: {
        return "c++11"; // TODO: Workaround for qbs 1.4. Remove once 1.5 is out.
        if (hostBuild)
            return "c++11"; // TODO: What is the correct way to get this information?
        if (project.config.contains("c++1z"))
            return "c++1z";
        if (project.config.contains("c++14"))
            return "c++14";
        if (project.config.contains("c++11"))
            return "c++11";
        if (project.config.contains("c++98"))
            return "c++98";
        return undefined;
    }

    cpp.cxxStandardLibrary: {
        if (qbs.toolchain.contains("clang")) {
            if (qbs.targetOS.contains("darwin"))
                return "libc++";
            if (qbs.targetOS.contains("linux"))
                return "libstdc++";
        }
        return base;
    }

    cpp.defines: {
        var defines = [
            "QT_BUILDING_QT",
            "QT_MOC_COMPAT",
            "_USE_MATH_DEFINES",
            "QT_ASCII_CAST_WARNINGS",
            "QT_DEPRECATED_WARNINGS",
            "QT_DISABLE_DEPRECATED_BEFORE=0x040800",
            "QT_USE_QSTRINGBUILDER",
        ];
        if (cpp.platformDefines)
            defines = defines.concat(cpp.platformDefines);
        if (qbs.toolchain.contains("msvc"))
            defines.push("_SCL_SECURE_NO_WARNINGS");
        return defines;
    }

    cpp.includePaths: [
        project.qtbasePrefix + "mkspecs/" + project.targetMkspec,
        product.buildDirectory + "/.moc",
        product.buildDirectory + "/.uic",
        project.buildDirectory + "/include",
    ]

    Properties {
        condition: qbs.toolchain.contains("gcc") && project.rpath
        cpp.sonamePrefix: qbs.targetOS.contains("darwin") ? "@rpath" : undefined
        cpp.rpaths: qbs.installRoot + "/lib"
    }

    Rule {
        name: "QtCoreMocRule"
        inputs: ["objcpp", "cpp", "hpp"]
        auxiliaryInputs: ["qt_plugin_metadata"]
        excludedAuxiliaryInputs: ["unmocable"]
        outputFileTags: ["hpp", "cpp", "unmocable"]
        outputArtifacts: {
            var mocinfo = QtMocScanner.apply(input);
            if (!mocinfo.hasQObjectMacro)
                return [];
            var artifact = { fileTags: ["unmocable"] };
            if (input.fileTags.contains("hpp")) {
                artifact.filePath = product.buildDirectory + "/.moc"
                        + "/moc_" + input.completeBaseName + ".cpp";
            } else {
                artifact.filePath = product.buildDirectory + "/.moc"
                          + '/' + input.completeBaseName + ".moc";
            }
            artifact.fileTags.push(mocinfo.mustCompile ? "cpp" : "hpp");
            if (mocinfo.hasPluginMetaDataMacro)
                artifact.explicitlyDependsOn = ["qt_plugin_metadata"];
            return [artifact];
        }
        prepare: {
            var args = [
                //"--no-warnings",
            ];

            // TODO: More or less copy from moc.js in qbs

            var defines = product.moduleProperty("cpp", "defines");
            var compilerDefines = product.moduleProperty("cpp", "compilerDefines");
            if (compilerDefines)
                defines = defines.concat(compilerDefines);
            defines.forEach(function(define) {
                if (!define)
                    return;
                args.push("-D");
                args.push(define);
            });

            var includes = product.moduleProperty("cpp", "includePaths");
            includes.forEach(function(include) {
                if (!include)
                    return;
                args.push("-I");
                args.push(include);
            });

            var allOutputs = [];
            if (outputs.cpp)
                allOutputs = allOutputs.concat(outputs.cpp);
            if (outputs.hpp)
                allOutputs = allOutputs.concat(outputs.hpp);
            if (!allOutputs.length)
                throw "QtLibrary.moc: outputs is empty.";

            var commands = [];
            allOutputs.forEach(function(output) {
                var cmd = new Command(project.buildDirectory + "/bin/moc", args.concat([
                    input.fileName, "-o", output.filePath,
                ]));
                cmd.workingDirectory = FileInfo.path(input.filePath);
                cmd.description = "moc " + input.fileName;
                cmd.highlight = "codegen";
                commands.push(cmd);
            });
            return commands;
        }
    }

    Rule {
        inputs: "rcc"
        Artifact {
            fileTags: "cpp"
            filePath: product.buildDirectory + "/.rcc/" + input.baseName + "_rcc.cpp"
        }
        prepare: {
            var cmd = new Command(project.buildDirectory  + "/bin/rcc", [
                input.filePath,
                "--name", input.baseName,
                "-o", output.filePath,
            ]);
            cmd.description = "rcc " + input.fileName;
            cmd.highlight = "codegen";
            return cmd;
        }
    }

    Rule {
        inputs: "uic"
        Artifact {
            fileTags: "hpp"
            filePath: product.buildDirectory + "/.uic/ui_" + input.baseName + ".h"
        }
        prepare: {
            var installRoot = product.moduleProperty("qbs", "installRoot");
            var installPrefix = product.moduleProperty("qbs", "installPrefix");
            var cmd = new Command(FileInfo.joinPaths(installRoot, installPrefix, "bin/uic"),
                ["-o", output.filePath, input.filePath]);
            cmd.description = "uic " + input.fileName;
            cmd.highlight = "codegen";
            return cmd;
        }
    }

    FileTagger {
        patterns: "*.qrc"
        fileTags: "rcc"
    }

    FileTagger {
        patterns: "*.ui"
        fileTags: "uic"
    }
}

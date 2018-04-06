import qbs.TextFile
import QtCorePrivateConfig

Module {
    FileTagger {
        patterns: ["*.tracepoints"]
        fileTags: ["tracepoint_provider"]
    }
    property stringList supportedBackends: ["lttng", "etw"]
    Rule {
        condition: supportedBackends.every(function(feature) {
            return !QtCorePrivateConfig[feature];
        })
        inputs: ["tracepoint_provider"]
        Artifact {
            filePath: input.baseName + "_tracepoints_p.h"
            fileTags: ["hpp"]
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "Creating tracepoint files";
            cmd.sourceCode = function() {
                var outFile = new TextFile(outputs["hpp"][0].filePath, TextFile.WriteOnly);
                outFile.writeLine("#include <private/qtrace_p.h>");
                outFile.close();
            };
            return cmd;
        }
    }
    Rule {
        condition: supportedBackends.some(function(feature) {
            return QtCorePrivateConfig[feature];
        })
        inputs: ["tracepoint_provider"]
        Artifact {
            filePath: input.baseName + "_tracepoints_p.h"
            fileTags: ["hpp"]
        }
        Artifact {
            filePath: input.baseName + "_tracepoints.cpp"
            fileTags: ["cpp"]
        }
        prepare: {
            // TODO: implement properly
            var cmd = new JavaScriptCommand();
            cmd.description = "Creating tracepoint files";
            cmd.sourceCode = function() {
                var outFile = new TextFile(outputs["hpp"][0].filePath, TextFile.WriteOnly);
                outFile.writeLine("#include <private/qtrace_p.h>");
                outFile.close();
                outFile = new TextFile(outputs["cpp"][0].filePath, TextFile.WriteOnly);
                outFile.writeLine("#include \"qtrace_p.h\"");
                outFile.close();
            };
            return cmd;
        }
    }
}

import qbs.FileInfo

Module {
    property string outputPath: "./"
    property var params: []
    Rule {
        inputs: ["shader"]
        outputFileTags: ["hpp"]
        outputArtifacts: input.fxc.params.map(function(p) {
                return {
                    filePath: FileInfo.joinPaths(input.fxc.outputPath, p.output),
                    fileTags: ["hpp"],
                };
        })
        prepare: {
            var paramByOutput = input.fxc.params.reduce(function(m, p) {
                m[p.output] = p;
                return m;
            }, {});
            return outputs.hpp.map(function(output) {
                var p = paramByOutput[output.fileName];
                var cmd = new Command("fxc.exe", [
                    "/nologo",
                    "/E", p.entry,
                    "/T", p.type,
                    "/Fh", FileInfo.toNativeSeparators(output.filePath),
                    FileInfo.toNativeSeparators(input.filePath)
                ]);
                cmd.description = "compiling Direct3D shader " + p.entry;
                return cmd;
            });
        }
    }
    FileTagger {
        patterns: ["*.vs", "*.ps", "*.hlsl"]
        fileTags: ["shader"]
    }
}

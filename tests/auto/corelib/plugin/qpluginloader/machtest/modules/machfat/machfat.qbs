import qbs

Module {
    property stringList additionalLipoArgs: []

    Rule {
        multiplex: true
        inputsFromDependencies: ["dynamiclibrary"]

        Artifact {
            filePath: product.targetName + product.moduleProperty("cpp", "dynamicLibrarySuffix")
            fileTags: ["dynamiclibrary_fat"]
        }

        prepare: {
            var args = ["-create", "-output", output.filePath];
            for (var i = 0; i < inputs.dynamiclibrary.length; ++i) {
                var fn = inputs.dynamiclibrary[i].fileName;
                var fp = inputs.dynamiclibrary[i].filePath;
                args.push("-arch", fn.split(".")[1], fp);
            }
            args = args.concat(product.moduleProperties("machfat", "additionalLipoArgs"));
            var cmd = new Command("lipo", args);
            return cmd;
        }
    }
}

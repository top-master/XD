import qbs
import qbs.FileInfo
import qbs.TextFile
import QtGuiConfig

Project {
    name: "QtANGLE_" + shortName
    condition: QtGuiConfig.combined_angle_lib
    property string shortName: "combined_lib"
    ANGLELibGLESv2 {
        targetName: "QtANGLE"
        Depends { name: "QtANGLE_combined_def" }
        cpp.defines: base.concat(
            "EGLAPI=",
            "GL_APICALL=",
            "GL_GLEXT_PROTOTYPES=",
            "LIBEGL_IMPLEMENTATION"
        )
        Group {
            prefix: project.angleDir + "/src/libEGL/"
            files: [
                "libEGL.cpp",
                "resource.h",
            ]
        }
        cpp.linkerFlags: [
            (qbs.toolchain.contains("msvc") ? "/DEF:" : "") + QtANGLE_combined_def.defFile,
        ]
    }
    Product {
        name: "QtANGLE_combined_def"
        type: ["def"]
        Export {
            property string defFile: FileInfo.joinPaths(product.buildDirectory, "combined.def")
        }
        Group {
            files: ["libGLESv2", "libEGL"].map(function(lib) {
                return project.angleDir + "/src/" + lib + "/" + lib
                    + (qbs.toolchain.contains("mingw") ? "_mingw32" : "")
                    + ".def";
            })
            fileTags: ["src_def"]
        }
        Rule {
            multiplex: true
            inputs: ["src_def"]
            Artifact {
                filePath: "combined.def"
                fileTags: ["def"]
            }
            prepare: {
                var cmd = new JavaScriptCommand();
                cmd.description = "combining module definitions";
                cmd.sourceCode = function() {
                    function readSymbols(filePath) {
                        var result = [];
                        var f = new TextFile(filePath, TextFile.ReadOnly);
                        while (!f.atEof()) {
                            var line = f.readLine();
                            if (line.match(/\bLIBRARY\b|\bEXPORTS\b|^\s*;|^\s*$/))
                                continue;
                            var entry = line.replace(/\s/g, '').split('@');
                            result.push(entry[0]);
                        }
                        f.close();
                        return result;
                    }

                    function mergedSymbols() {
                        return inputs.src_def.map(function(input) {
                            return {
                                filePath: input.filePath,
                                symbols: readSymbols(input.filePath),
                            };
                        }).reduce(function(txt, e) {
                            txt += ";\n";
                            txt += ";   Generated from:\n";
                            txt += ";   " + e.filePath + "\n";
                            txt += "    " + e.symbols.join("\n    ");
                            txt += "\n";
                            return txt;
                        }, "");
                    }

                    var out = new TextFile(output.filePath, TextFile.WriteOnly);
                    out.writeLine("LIBRARY");
                    out.writeLine("EXPORTS");
                    out.write(mergedSymbols());
                    out.close();
                };
                return [cmd];
            }
        }
    }
}

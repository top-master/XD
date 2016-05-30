import qbs
import qbs.File

QtProduct {
    name: "mylib2"
    targetName: qbs.targetOS.contains("windows") ? "mylib2" : "mylib"
    type: ["dynamiclibrary", "dllcopy"]
    destinationDirectory: project.buildDirectory + "/qlibrary_test"
    version: "2"
    property bool isWindows: qbs.targetOS.contains("windows")

    Depends { name: "Qt.core" }

    Properties {
        condition: !isWindows
        cpp.internalVersion: "2"
    }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.defines: base.concat(["WIN32_MSVC"])
    }

    Rule {
        inputs: ["dynamiclibrary"]
        Artifact {
            filePath: product.destinationDirectory
                      + (product.isWindows ? "/mylib.dl2" : "/libmylib.so2")
            fileTags: ["dllcopy"]
        }
        Artifact {
            filePath: product.destinationDirectory + "/system.qt.test.mylib."
                      + (product.isWindows ? "dll" : "so")
            fileTags: ["dllcopy"]
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "Creating test plugin copies";
            cmd.sourceCode = function() {
                for (var i = 0; i < outputs.dllcopy.length; ++i)
                    File.copy(input.filePath, outputs.dllcopy[i].filePath);
            };
            return [cmd];
        }
    }

    files: [
        "mylib.c",
    ]
}

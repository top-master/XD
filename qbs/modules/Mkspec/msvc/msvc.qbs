import qbs

Module {
    Depends { name: "cpp" }
    cpp.commonCompilerFlags: {
        var flags = [];
        if (cpp.compilerVersionMajor >= 19) {
            flags.push("/Zc:strictStrings");
        }
        return flags;
    }
    cpp.cxxFlags: {
        var flags = [];
        if (cpp.compilerVersionMajor >= 18) {
            flags.push("/Zc:rvalueCast", "/Zc:inline");
        }
        if (cpp.compilerVersionMajor >= 19) {
            flags.push("/Zc:throwingNew");
            if (cpp.compilerVersionMinor >= 1) {
                flags.push("/Zc:referenceBinding");
            }
        }
        return flags;
    }
}

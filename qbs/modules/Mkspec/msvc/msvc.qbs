import qbs
import qbs.FileInfo

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
    property string windowsKitDir: cpp.buildEnv["WindowsSdkDir"]
    property string windowsKitVersion: {
        var s = cpp.buildEnv["WindowsSDKVersion"];
        if (s)
            return s.replace(/\\$/, "");
        return undefined;
    }
    property string windowsKitIncludeDir: FileInfo.joinPaths(windowsKitDir,
                                                             "Include", windowsKitVersion)
    Properties {
        condition: product.targetsUWP
        cpp.windowsApiFamily: "pc"
        cpp.includePaths: [
            "shared",
            "ucrt",
            "um",
            "winrt",
        ].map(function(s) { return FileInfo.joinPaths(windowsKitIncludeDir, s); })
    }
    cpp.dynamicLibraries: {
        var libs = [];
        if (product.targetsUWP) {
            libs.push("runtimeobject");
        }
        return libs;
    }
}

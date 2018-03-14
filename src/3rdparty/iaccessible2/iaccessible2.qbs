import qbs
import qbs.FileInfo

Product {
    name: "qt_iaccessible2"
    Export {
        Depends { name: "cpp" }
        readonly property string _arch: {
            if (qbs.architecture === "x86_64")
                return "amd64";
            if (qbs.architecture === "x86")
                return qbs.architecture;
            throw new Error("Could not detect architecture from qbs.architecture");
        }
        readonly property string _midlGenerated: FileInfo.joinPaths("generated", _arch)
        cpp.includePaths: [_midlGenerated]
        cpp.dynamicLibraries: ["rpcrt4"]
        Group {
            files: [
                "idl/ia2_api_all.idl",
            ]
        }
        Group {
            prefix: product.qt_iaccessible2._midlGenerated + "/"
            files: [
                "ia2_api_all.h",
                "ia2_api_all_i.c",
            ]
        }
    }
}

import qbs

Module {
    Depends { name: "cpp" }
    cpp.defines: ["_REENTRANT"]
    cpp.dynamicLibraries: "pthread"
}

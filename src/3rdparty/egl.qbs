import qbs

PkgConfigDependency {
    name: "egl"
    condition: qbs.targetOS.contains("android") || project.angle || found

    Depends { name: "libEGL"; condition: project.angle }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: project.angle ? project.qtbasePrefix + "src/3rdparty/angle/include"
                                        : product.includePaths
        cpp.dynamicLibraries: project.angle ? project.libDirectory + "/libEGL" + (qbs.enableDebugCode ? 'd' : '') + ".lib"
                                            : product.dynamicLibraries
    }
}

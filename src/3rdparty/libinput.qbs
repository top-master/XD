import qbs

PkgConfigDependency {
    name: "libinput"

    Export {
        condition: product.runProbe
        Depends { name: "cpp" }
        cpp.defines: {
            var version = product.version.split('.');
            return [
                "QT_LIBINPUT_VERSION_MAJOR=" + version[0],
                "QT_LIBINPUT_VERSION_MINOR=" + version[1],
            ];
        }
        cpp.dynamicLibraries: product.dynamicLibraries
        cpp.libraryPaths: product.libraryPaths
        cpp.includePaths: product.includePaths
    }
}

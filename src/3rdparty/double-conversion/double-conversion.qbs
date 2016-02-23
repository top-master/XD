import qbs

PkgConfigDependency {
    name: "double-conversion"
    type: project.doubleConversion ? ["staticlibrary"] : []
    condition: true
    destinationDirectory: project.libDirectory
    runProbe: !project.doubleConversion && project.system_doubleConversion

    Export {
        Depends { name: "cpp" }
        cpp.defines: !project.doubleConversion && !project.system_doubleConversion
                     ? ["QT_NO_DOUBLECONVERSION"] : []
        cpp.dynamicLibraries: !project.doubleConversion && project.system_doubleConversion
                              && !product.found ? ["double-conversion"] : []
        cpp.includePaths: project.doubleConversion ? [product.sourceDirectory + "/include"] : []
    }

    Depends { name: "cpp" }
    cpp.includePaths: [product.sourceDirectory + "/include/double-conversion"]

    Group {
        name: "sources"
        condition: project.doubleConversion
        files: [
            "bignum.cc",
            "bignum.h",
            "bignum-dtoa.cc",
            "bignum-dtoa.h",
            "cached-powers.cc",
            "cached-powers.h",
            "diy-fp.cc",
            "diy-fp.h",
            "double-conversion.cc",
            "fast-dtoa.cc",
            "fast-dtoa.h",
            "fixed-dtoa.cc",
            "fixed-dtoa.h",
            "ieee.h",
            "strtod.cc",
            "strtod.h",
            "include/double-conversion/double-conversion.h",
            "include/double-conversion/utils.h",
        ]
    }
}

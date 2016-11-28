import qbs

PkgConfigDependency {
    name: "libpng"
    type: project.system_png ? "hpp" : "staticlibrary"
    condition: project.system_png ? found : true
    destinationDirectory: project.libDirectory

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: project.system_png ? product.includePaths : [
            project.qtbasePrefix + "src/3rdparty/libpng",
        ]
    }

    Depends { name: "Android.ndk"; condition: qbs.targetOS.contains("android") }
    Depends { name: "cpp" }
    Depends { name: "zlib" }

    cpp.defines: ["PNG_ARM_NEON_OPT=0"]

    Group {
        name: "sources"
        files: [
            "pngwutil.c",
            "png.c",
            "png.h",
            "pngconf.h",
            "pngdebug.h",
            "pngerror.c",
            "pngget.c",
            "pnginfo.h",
            "pnglibconf.h",
            "pngmem.c",
            "pngpread.c",
            "pngpriv.h",
            "pngread.c",
            "pngrio.c",
            "pngrtran.c",
            "pngrutil.c",
            "pngset.c",
            "pngstruct.h",
            "pngtrans.c",
            "pngwio.c",
            "pngwrite.c",
            "pngwtran.c",
        ]
    }
}

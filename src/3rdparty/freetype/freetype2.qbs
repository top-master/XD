import qbs

PkgConfigDependency {
    name: "freetype2"
    type: project.system_freetype ? "hpp" : "staticlibrary"
    condition: project.system_freetype ? found : true
    destinationDirectory: project.libDirectory

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: project.system_freetype ? product.includePaths : [
            project.qtbasePrefix + "src/3rdparty/freetype/include",
        ]
    }

    Depends { name: "Android.ndk"; condition: qbs.targetOS.contains("android") }
    Depends { name: "cpp" }

    cpp.defines: [
        "FT2_BUILD_LIBRARY",
    ].concat(base)

    cpp.includePaths: {
        var includePaths = base;
        includePaths.push(project.qtbasePrefix + "src/3rdparty/freetype/include");
        if (qbs.targetOS.contains("unix"))
            includePaths.push(project.qtbasePrefix + "src/3rdparty/freetype/builds/unix");
        else
            includePaths.push(project.qtbasePrefix + "src/3rdparty/freetype/include/config");
        return includePaths;
    }

    Group {
        condition: !project.system_freetype
        name: "sources"
        prefix: "src/"
        files: [
            "base/ftbase.c",
            "base/ftbbox.c",
            "base/ftdebug.c",
            "base/ftglyph.c",
            "base/ftlcdfil.c",
            "base/ftinit.c",
            "base/ftmm.c",
            "base/fttype1.c",
            "base/ftsynth.c",
            "base/ftbitmap.c",
            "bdf/bdf.c",
            "cache/ftcache.c",
            "cff/cff.c",
            "cid/type1cid.c",
            "gzip/ftgzip.c",
            "pcf/pcf.c",
            "pfr/pfr.c",
            "psaux/psaux.c",
            "pshinter/pshinter.c",
            "psnames/psmodule.c",
            "raster/raster.c",
            "sfnt/sfnt.c",
            "smooth/smooth.c",
            "truetype/truetype.c",
            "type1/type1.c",
            "type42/type42.c",
            "winfonts/winfnt.c",
            "lzw/ftlzw.c",
            "otvalid/otvalid.c",
            "autofit/autofit.c",
        ]
    }

    Group {
        name: "sources_unix"
        condition: !project.system_freetype && qbs.targetOS.contains("unix")
        files: [
            "builds/unix/ftsystem.c",
        ]
    }

    Group {
        name: "sources_windows"
        condition: !project.system_freetype && qbs.targetOS.contains("windows")
        files: [
            "src/base/ftsystem.c",
        ]
    }
}

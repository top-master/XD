import qbs

Project {
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]
    Product {
        name: "qt_freetype"
        condition: QtGuiPrivateConfig.freetype
        Depends { name: "QtGuiPrivateConfig" }
        Export {
            Depends { name: "QtGuiPrivateConfig" }
            Depends {
                condition: !QtGuiPrivateConfig.system_freetype
                name: "bundled_freetype"
            }
            Depends {
                condition: QtGuiPrivateConfig.system_freetype
                name: "Freetype"
            }
        }
    }
    QtStaticLibrary {
        name: "bundled_freetype"
        targetName: "qtfreetype"
        condition: !QtGuiPrivateConfig.system_freetype
        Depends { name: "QtGuiPrivateConfig" }
        Depends { name: "qt_libpng" }
        Depends { name: "qt_zlib" }
        Depends { name: "cpp" }
        cpp.enableExceptions: false
        cpp.enableRtti: false
        cpp.includePaths: {
            var result = ["include"];
            if (!qbs.targetOS.contains("windows"))
                result.push("builds/unix");
            return result;
        }
        cpp.warningLevel: "none"
        cpp.defines: [
            "FT2_BUILD_LIBRARY",
            "FT_CONFIG_OPTION_SYSTEM_ZLIB",
            "FT_CONFIG_OPTION_USE_PNG",
            "TT_CONFIG_OPTION_SUBPIXEL_HINTING",
        ]
        files: [
            "src/autofit/afangles.c",
            "src/autofit/afdummy.c",
            "src/autofit/afglobal.c",
            "src/autofit/afhints.c",
            "src/autofit/aflatin.c",
            "src/autofit/afloader.c",
            "src/autofit/afmodule.c",
            "src/autofit/autofit.c",
            "src/base/ftbase.c",
            "src/base/ftbbox.c",
            "src/base/ftbitmap.c",
            "src/base/ftdebug.c",
            "src/base/ftfntfmt.c",
            "src/base/ftglyph.c",
            "src/base/ftinit.c",
            "src/base/ftlcdfil.c",
            "src/base/ftmm.c",
            "src/base/ftsynth.c",
            "src/base/fttype1.c",
            "src/bdf/bdf.c",
            "src/cache/ftcache.c",
            "src/cff/cff.c",
            "src/cid/type1cid.c",
            "src/gzip/ftgzip.c",
            "src/lzw/ftlzw.c",
            "src/otvalid/otvalid.c",
            "src/otvalid/otvbase.c",
            "src/otvalid/otvcommn.c",
            "src/otvalid/otvgdef.c",
            "src/otvalid/otvgpos.c",
            "src/otvalid/otvgsub.c",
            "src/otvalid/otvjstf.c",
            "src/otvalid/otvmod.c",
            "src/pcf/pcf.c",
            "src/pfr/pfr.c",
            "src/psaux/psaux.c",
            "src/pshinter/pshinter.c",
            "src/psnames/psmodule.c",
            "src/raster/raster.c",
            "src/sfnt/sfnt.c",
            "src/smooth/smooth.c",
            "src/truetype/truetype.c",
            "src/type1/type1.c",
            "src/type42/type42.c",
            "src/winfonts/winfnt.c",
        ]
        Group {
            condition: qbs.targetOS.contains("windows")
            files: [
                "src/base/ftsystem.c"
            ]
        }
        Group {
            condition: !qbs.targetOS.contains("windows")
            files: [
                "builds/unix/ftsystem.c"
            ]
        }
        Export {
            Depends { name: "cpp" }
            cpp.includePaths: ["include"]
        }
    }
}

import qbs
import QtGuiPrivateConfig

Project {
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]
    Product {
        name: "qt_libpng"
        condition: QtGuiPrivateConfig.png
        Export {
            Depends {
                condition: !QtGuiPrivateConfig.system_png
                name: "bundled_libpng"
            }
            Depends {
                condition: QtGuiPrivateConfig.system_png
                name: "Libpng"
            }
        }
    }

    QtStaticLibrary {
        name: "bundled_libpng"
        condition: QtGuiPrivateConfig.png && !QtGuiPrivateConfig.system_png
        qt_warnings.warningClean: false
        Depends { name: "cpp" }
        Depends { name: "qt_zlib" }
        cpp.defines: ["PNG_ARM_NEON_OPT=0"]
        Export {
            Depends { name: "cpp" }
            cpp.includePaths: [product.sourceDirectory]
        }
        files: [
            "png.c",
            "pngerror.c",
            "pngget.c",
            "pngmem.c",
            "pngpread.c",
            "pngread.c",
            "pngrio.c",
            "pngrtran.c",
            "pngrutil.c",
            "pngset.c",
            "pngtrans.c",
            "pngwio.c",
            "pngwrite.c",
            "pngwtran.c",
            "pngwutil.c",
        ]
    }
}

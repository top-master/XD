import qbs
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtGui"
    simpleName: "gui"
    prefix: project.qtbasePrefix + "src/gui/"

    Product {
        name: root.privateName
        profiles: project.targetProfiles
        type: "hpp"
        Depends { name: root.moduleName }
        Export {
            Depends { name: "cpp" }
            cpp.defines: root.defines
            cpp.includePaths: root.includePaths
        }
    }

    QtHeaders {
        name: root.headersName
        sync.module: root.name
        Depends { name: "QtCoreHeaders" }
        ModuleHeaders { fileTags: "hpp_syncable" }
    }

    QtModule {
        name: root.moduleName
        condition: project.gui
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "gui.pro"

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: root.publicIncludePaths
            Depends { name: "glesv2"; condition: project.opengles2 }
        }

        Depends { name: root.headersName }
        Depends { name: "Qt"; submodules: ["core", "core-private"] }

        Depends { name: "freetype2" }
        Depends { name: "gl"; condition: project.opengl && !project.opengles2 && !qbs.targetOS.contains("windows") && !(qbs.targetOS.contains("darwin") && !qbs.targetOS.contains("osx")) }
        Depends { name: "glesv2"; condition: project.opengles2 }
        Depends { name: "libjpeg" }
        Depends { name: "libpng" }
        Depends { name: "zlib" }

        cpp.defines: [
            "QT_BUILD_GUI_LIB",
        ].concat(base);

        cpp.dynamicLibraries: {
            var dynamicLibraries = base;
            if (qbs.targetOS.contains("windows")) {
                dynamicLibraries.push("user32");
                dynamicLibraries.push("ole32");
                if (!qbs.targetOS.contains("winrt"))
                    dynamicLibraries.push("gdi32");
                if (project.opengl) {
                    dynamicLibraries.push("opengl32");
                }
            }
            if (qbs.targetOS.contains("darwin")) {
                dynamicLibraries.push("objc"); // for qaccessiblecache_mac.mm
            }
            return dynamicLibraries;
        }

        cpp.frameworks: {
            var frameworks = base;
            if (qbs.targetOS.contains("darwin")) {
                frameworks.push("CoreFoundation");
            }
            return frameworks;
        }

        cpp.includePaths: root.includePaths.concat(base)

        ModuleHeaders {
            excludeFiles: {
                var excludeFiles = ["doc/**"];
                if (project.opengl == "es2")
                    excludeFiles.push("opengl/qopengltimerquery.h");
                return excludeFiles;
            }
        }

        Properties {
            condition: project.precompiledHeaders
            cpp.cxxPrecompiledHeader: "kernel/qt_gui_pch.h"
        }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "accessible/qaccessiblebridge.cpp",
                "accessible/qaccessiblecache.cpp",
                "accessible/qaccessible.cpp",
                "accessible/qaccessibleobject.cpp",
                "accessible/qaccessibleplugin.cpp",
                "accessible/qplatformaccessibility.cpp",
                "animation/qguivariantanimation.cpp",
                "image/qbitmap.cpp",
                "image/qbmphandler.cpp",
                "image/qgifhandler.cpp",
                "image/qicon.cpp",
                "image/qiconengine.cpp",
                "image/qiconengineplugin.cpp",
                "image/qiconloader.cpp",
                "image/qimage_conversions.cpp",
                "image/qimage.cpp",
                "image/qimageiohandler.cpp",
                "image/qimagepixmapcleanuphooks.cpp",
                "image/qimagereader.cpp",
                "image/qimagewriter.cpp",
                "image/qjpeghandler.cpp",
                "image/qmovie.cpp",
                "image/qnativeimage.cpp",
                "image/qpaintengine_pic.cpp",
                "image/qpicture.cpp",
                "image/qpictureformatplugin.cpp",
                "image/qpixmap_blitter.cpp",
                "image/qpixmapcache.cpp",
                "image/qpixmap.cpp",
                "image/qpixmap_raster.cpp",
                "image/qplatformpixmap.cpp",
                "image/qpnghandler.cpp",
                "image/qppmhandler.cpp",
                "image/qxbmhandler.cpp",
                "image/qxpmhandler.cpp",
                "itemmodels/qstandarditemmodel.cpp",
                "kernel/qclipboard.cpp",
                "kernel/qcursor.cpp",
                "kernel/qdnd.cpp",
                "kernel/qdrag.cpp",
                "kernel/qevent.cpp",
                "kernel/qgenericplugin.cpp",
                "kernel/qgenericpluginfactory.cpp",
                "kernel/qguiapplication.cpp",
                "kernel/qguivariant.cpp",
                "kernel/qhighdpiscaling.cpp",
                "kernel/qinputdevicemanager.cpp",
                "kernel/qinputmethod.cpp",
                "kernel/qkeymapper.cpp",
                "kernel/qkeysequence.cpp",
                "kernel/qoffscreensurface.cpp",
                "kernel/qopenglcontext.cpp",
                "kernel/qopenglwindow.cpp",
                "kernel/qpaintdevicewindow.cpp",
                "kernel/qpalette.cpp",
                "kernel/qpixelformat.cpp",
                "kernel/qplatformclipboard.cpp",
                "kernel/qplatformcursor.cpp",
                "kernel/qplatformdialoghelper.cpp",
                "kernel/qplatformdrag.cpp",
                "kernel/qplatformgraphicsbuffer.cpp",
                "kernel/qplatformgraphicsbufferhelper.cpp",
                "kernel/qplatforminputcontext.cpp",
                "kernel/qplatforminputcontextfactory.cpp",
                "kernel/qplatforminputcontextplugin.cpp",
                "kernel/qplatformintegration.cpp",
                "kernel/qplatformintegrationfactory.cpp",
                "kernel/qplatformintegrationplugin.cpp",
                "kernel/qplatformmenu.cpp",
                "kernel/qplatformnativeinterface.cpp",
                "kernel/qplatformoffscreensurface.cpp",
                "kernel/qplatformopenglcontext.cpp",
                "kernel/qplatformscreen.cpp",
                "kernel/qplatformservices.cpp",
                "kernel/qplatformsessionmanager.cpp",
                "kernel/qplatformsharedgraphicscache.cpp",
                "kernel/qplatformsurface.cpp",
                "kernel/qplatformsystemtrayicon.cpp",
                "kernel/qplatformtheme.cpp",
                "kernel/qplatformthemefactory.cpp",
                "kernel/qplatformthemeplugin.cpp",
                "kernel/qplatformwindow.cpp",
                "kernel/qrasterwindow.cpp",
                "kernel/qscreen.cpp",
                "kernel/qsessionmanager.cpp",
                "kernel/qshapedpixmapdndwindow.cpp",
                "kernel/qshortcutmap.cpp",
                "kernel/qsimpledrag.cpp",
                "kernel/qstylehints.cpp",
                "kernel/qsurface.cpp",
                "kernel/qsurfaceformat.cpp",
                "kernel/qtouchdevice.cpp",
                "kernel/qwindow.cpp",
                "kernel/qwindowsysteminterface.cpp",
                "math3d/qgenericmatrix.cpp",
                "math3d/qmatrix4x4.cpp",
                "math3d/qquaternion.cpp",
                "math3d/qvector2d.cpp",
                "math3d/qvector3d.cpp",
                "math3d/qvector4d.cpp",
                "opengl/qopengl2pexvertexarray.cpp",
                "opengl/qopenglbuffer.cpp",
                "opengl/qopengl.cpp",
                "opengl/qopenglcustomshaderstage.cpp",
                "opengl/qopengldebug.cpp",
                "opengl/qopenglengineshadermanager.cpp",
                "opengl/qopenglframebufferobject.cpp",
                "opengl/qopenglfunctions.cpp",
                "opengl/qopenglgradientcache.cpp",
                "opengl/qopenglpaintdevice.cpp",
                "opengl/qopenglpaintengine.cpp",
                "opengl/qopenglpixeltransferoptions.cpp",
                "opengl/qopenglshaderprogram.cpp",
                "opengl/qopengltextureblitter.cpp",
                "opengl/qopengltexturecache.cpp",
                "opengl/qopengltexture.cpp",
                "opengl/qopengltextureglyphcache.cpp",
                "opengl/qopengltexturehelper.cpp",
                "opengl/qopenglversionfunctions.cpp",
                "opengl/qopenglversionfunctionsfactory.cpp",
                "opengl/qopenglvertexarrayobject.cpp",
                "opengl/qtriangulatingstroker.cpp",
                "opengl/qtriangulator.cpp",
                "painting/qbackingstore.cpp",
                "painting/qbezier.cpp",
                "painting/qblendfunctions.cpp",
                "painting/qblittable.cpp",
                "painting/qbrush.cpp",
                "painting/qcolor.cpp",
                "painting/qcolor_p.cpp",
                "painting/qcompositionfunctions.cpp",
                "painting/qcosmeticstroker.cpp",
                "painting/qcssutil.cpp",
                "painting/qdrawhelper.cpp",
                "painting/qemulationpaintengine.cpp",
                "painting/qgammatables.cpp",
                "painting/qgrayraster.c",
                "painting/qimagescale.cpp",
                "painting/qmatrix.cpp",
                "painting/qmemrotate.cpp",
                "painting/qoutlinemapper.cpp",
                "painting/qpagedpaintdevice.cpp",
                "painting/qpagelayout.cpp",
                "painting/qpagesize.cpp",
                "painting/qpaintdevice.cpp",
                "painting/qpaintengine_blitter.cpp",
                "painting/qpaintengine.cpp",
                "painting/qpaintengineex.cpp",
                "painting/qpaintengine_raster.cpp",
                "painting/qpainter.cpp",
                "painting/qpainterpath.cpp",
                "painting/qpathclipper.cpp",
                "painting/qpathsimplifier.cpp",
                "painting/qpdf.cpp",
                "painting/qpdfwriter.cpp",
                "painting/qpen.cpp",
                "painting/qplatformbackingstore.cpp",
                "painting/qpolygon.cpp",
                "painting/qrasterizer.cpp",
                "painting/qregion.cpp",
                "painting/qstroker.cpp",
                "painting/qtextureglyphcache.cpp",
                "painting/qtransform.cpp",
                "text/qabstracttextdocumentlayout.cpp",
                "text/qcssparser.cpp",
                "text/qdistancefield.cpp",
                "text/qfont.cpp",
                "text/qfontdatabase.cpp",
                "text/qfontengine.cpp",
                "text/qfontengineglyphcache.cpp",
                "text/qfontengine_qpf2.cpp",
                "text/qfontmetrics.cpp",
                "text/qfontsubset.cpp",
                "text/qfragmentmap.cpp",
                "text/qglyphrun.cpp",
                "text/qplatformfontdatabase.cpp",
                "text/qrawfont.cpp",
                "text/qstatictext.cpp",
                "text/qsyntaxhighlighter.cpp",
                "text/qtextcursor.cpp",
                "text/qtextdocument.cpp",
                "text/qtextdocumentfragment.cpp",
                "text/qtextdocumentlayout.cpp",
                "text/qtextdocument_p.cpp",
                "text/qtextdocumentwriter.cpp",
                "text/qtextengine.cpp",
                "text/qtextformat.cpp",
                "text/qtexthtmlparser.cpp",
                "text/qtextimagehandler.cpp",
                "text/qtextlayout.cpp",
                "text/qtextlist.cpp",
                "text/qtextobject.cpp",
                "text/qtextodfwriter.cpp",
                "text/qtextoption.cpp",
                "text/qtexttable.cpp",
                "text/qzip.cpp",
                "util/qabstractlayoutstyleinfo.cpp",
                "util/qdesktopservices.cpp",
                "util/qgridlayoutengine.cpp",
                "util/qlayoutpolicy.cpp",
                "util/qvalidator.cpp",
            ]
        }

        Group {
            name: "sources_no_pch"
            files: ["image/qimage_compat.cpp"]
            cpp.cxxPrecompiledHeader: undefined
        }

        Group {
            name: "sources_darwin"
            condition: qbs.targetOS.contains("darwin")
            prefix: root.prefix
            files: [
                "accessible/qaccessiblecache_mac.mm"
            ]
        }

        Group {
            name: "sources_windows"
            condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")
            prefix: root.prefix
            files: [
                "image/qpixmap_win.cpp",
            ]
        }

        Group {
            name: "sources_freetype"
            condition: !project.system_freetype
            prefix: root.prefix
            files: [
                "text/qfontengine_ft.cpp",
            ]
        }

        Group {
            name: "sources_opengles2"
            condition: project.opengles2
            prefix: root.prefix
            files: [
                "opengl/qopenglfunctions_es2.cpp",
            ]
        }

        Group {
            name: "sources_opengl"
            condition: project.opengl && !project.opengles2
            prefix: root.prefix
            files: [
                "opengl/qopenglfunctions_1_0.cpp",
                "opengl/qopenglfunctions_1_1.cpp",
                "opengl/qopenglfunctions_1_2.cpp",
                "opengl/qopenglfunctions_1_3.cpp",
                "opengl/qopenglfunctions_1_4.cpp",
                "opengl/qopenglfunctions_1_5.cpp",
                "opengl/qopenglfunctions_2_0.cpp",
                "opengl/qopenglfunctions_2_1.cpp",
                "opengl/qopenglfunctions_3_0.cpp",
                "opengl/qopenglfunctions_3_1.cpp",
                "opengl/qopenglfunctions_3_2_compatibility.cpp",
                "opengl/qopenglfunctions_3_2_core.cpp",
                "opengl/qopenglfunctions_3_3_compatibility.cpp",
                "opengl/qopenglfunctions_3_3_core.cpp",
                "opengl/qopenglfunctions_4_0_compatibility.cpp",
                "opengl/qopenglfunctions_4_0_core.cpp",
                "opengl/qopenglfunctions_4_1_compatibility.cpp",
                "opengl/qopenglfunctions_4_1_core.cpp",
                "opengl/qopenglfunctions_4_2_compatibility.cpp",
                "opengl/qopenglfunctions_4_2_core.cpp",
                "opengl/qopenglfunctions_4_3_compatibility.cpp",
                "opengl/qopenglfunctions_4_3_core.cpp",
                "opengl/qopenglfunctions_4_4_compatibility.cpp",
                "opengl/qopenglfunctions_4_4_core.cpp",
                "opengl/qopenglfunctions_4_5_compatibility.cpp",
                "opengl/qopenglfunctions_4_5_core.cpp",
                "opengl/qopengltimerquery.cpp",
            ]
        }

        Group {
            name: "sources_harfbuzzng"
            condition: project.harfbuzzng
            prefix: root.prefix
            files: [
                "text/qharfbuzzng.cpp",
            ]
        }

        Group {
            name: "sources_sse2"
            condition: project.sse2
            prefix: root.prefix
            cpp.cxxFlags: outer.concat(project.sse2Flags)
            cpp.cxxPrecompiledHeader: undefined
            files: [
                "image/qimage_sse2.cpp",
                "painting/qdrawhelper_sse2.cpp",
            ]
        }

        Group {
            name: "sources_ssse3"
            condition: project.ssse3
            prefix: root.prefix
            cpp.cxxFlags: outer.concat(project.ssse3Flags)
            cpp.cxxPrecompiledHeader: undefined
            files: [
                "image/qimage_ssse3.cpp",
                "painting/qdrawhelper_ssse3.cpp",
            ]
        }

        Group {
            name: "sources_sse4_1"
            condition: project.sse4_1
            prefix: root.prefix
            cpp.cxxFlags: outer.concat(project.sse4_1Flags)
            cpp.cxxPrecompiledHeader: undefined
            files: [
                "image/qimage_sse4.cpp",
                "painting/qimagescale_sse4.cpp",
                "painting/qdrawhelper_sse4.cpp",
            ]
        }

        Group {
            name: "sources_avx2"
            condition: project.avx2
            prefix: root.prefix
            cpp.cxxFlags: outer.concat(project.avx2Flags)
            cpp.cxxPrecompiledHeader: undefined
            files: [
                "image/qimage_avx2.cpp",
                "painting/qdrawhelper_avx2.cpp",
            ]
        }

        Group {
            name: "sources_mips_dspr2"
            condition: project.mips_dspr2
            prefix: root.prefix
            files: [
                "image/qimage_mips_dspr2.cpp",
                "painting/qdrawhelper_mips_dsp.cpp",
            ]
        }

        Group {
            name: "sources_neon"
            condition: project.neon
            prefix: root.prefix
            cpp.commonCompilerFlags: outer.concat(project.neonFlags)
            cpp.cxxPrecompiledHeader: undefined
            files: [
                "image/qimage_neon.cpp",
                "../3rdparty/pixman/pixman-arm-neon-asm.S",
                "painting/qdrawhelper_neon_asm.S",
                "painting/qdrawhelper_neon.cpp",
                "painting/qimagescale_neon.cpp"
            ]
        }
    }
}

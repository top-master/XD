import qbs
import QtGuiConfig
import QtGuiPrivateConfig

QtModuleProject {
    name: "QtGui"
    simpleName: "gui"
    conditionFunction: (function() {
        return Qt.global.privateConfig.gui;
    })

    QtHeaders {
        sync.classNames: ({
            "qevent.h": ["QtEvents"],
        })
        shadowBuildFiles:[
            project.qtbaseShadowDir + "/src/gui/qtgui-config.h",
            project.qtbaseShadowDir + "/src/gui/qtgui-config_p.h",
        ]
        Depends { name: "QtCoreHeaders" }
    }

    QtPrivateModule {
        Export {
            property var config: QtGuiPrivateConfig
        }
    }

    QtModuleTracepoints {}

    QtModule {
        Export {
            property var config: QtGuiConfig
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }
        Depends { name: "qt_zlib" }
        Depends { name: "cpufeatures" }
        Depends { name: "opengl" }

        cpp.enableExceptions: true
        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat([
            "QT_NO_FOREACH",
            "QT_NO_USING_NAMESPACE",
        ])

        Group {
            name: "Qt accessibility module"
            condition: QtGuiConfig.accessibility
            prefix: "accessible/"
            files: [
                "qaccessible.cpp",
                "qaccessible.h",
                "qaccessiblebridge.cpp",
                "qaccessiblebridge.h",
                "qaccessiblecache.cpp",
                "qaccessiblecache_p.h",
                "qaccessibleobject.cpp",
                "qaccessibleobject.h",
                "qaccessibleplugin.cpp",
                "qaccessibleplugin.h",
                "qplatformaccessibility.cpp",
                "qplatformaccessibility.h",
            ]
            Group {
                name: "macOS accessibility"
                condition: qbs.targetOS.contains("macos")
                files: [
                    "qaccessiblecache_mac.mm"
                ]
            }
        }
        Group {
            name: "animation"
            prefix: "animation/"
            files: [
                "qguivariantanimation.cpp",
            ]
        }
        cpp.frameworks: {
            var result = qbs.targetOS.contains("macos") ? ["Foundation", "AppKit"] : [];
            if (qbs.targetOS.contains("darwin"))
                result.push("CoreGraphics");
            return result;
        }
        Properties {
            condition: qbs.toolchain.contains("mingw") && qbs.architecture === "x86_64"
            cpp.commonCompilerFlags: ["-Wa,-mbig-obj"]
        }
        Properties {
            condition: qbs.targetOS.contains("windows") && !product.targetsUWP
            cpp.dynamicLibraries: ["ole32", "gdi32"]
        }

        Group {
            name: "precompiled header"
            files: ["kernel/qt_gui_pch.h"]
        }
        Group {
            name: "kernel"
            prefix: "kernel/"
            files: [
                "qclipboard.cpp",
                "qclipboard.h",
                "qcursor.cpp",
                "qcursor.h",
                "qcursor_p.h",
                "qdnd.cpp",
                "qdnd_p.h",
                "qdrag.cpp",
                "qdrag.h",
                "qevent.cpp",
                "qevent.h",
                "qevent_p.h",
                "qgenericplugin.cpp",
                "qgenericplugin.h",
                "qgenericpluginfactory.cpp",
                "qgenericpluginfactory.h",
                "qguiapplication.cpp",
                "qguiapplication.h",
                "qguiapplication_p.h",
                "qguivariant.cpp",
                "qhighdpiscaling.cpp",
                "qhighdpiscaling_p.h",
                "qinputdevicemanager.cpp",
                "qinputdevicemanager_p.h",
                "qinputdevicemanager_p_p.h",
                "qinputmethod.cpp",
                "qinputmethod.h",
                "qinputmethod_p.h",
                "qkeymapper.cpp",
                "qkeymapper_p.h",
                "qkeysequence.cpp",
                "qkeysequence.h",
                "qkeysequence_p.h",
                "qoffscreensurface.cpp",
                "qoffscreensurface.h",
                "qpaintdevicewindow.cpp",
                "qpaintdevicewindow.h",
                "qpaintdevicewindow_p.h",
                "qpalette.cpp",
                "qpalette.h",
                "qpixelformat.cpp",
                "qpixelformat.h",
                "qplatformclipboard.cpp",
                "qplatformclipboard.h",
                "qplatformcursor.cpp",
                "qplatformcursor.h",
                "qplatformdialoghelper.cpp",
                "qplatformdialoghelper.h",
                "qplatformdrag.cpp",
                "qplatformdrag.h",
                "qplatformgraphicsbuffer.cpp",
                "qplatformgraphicsbuffer.h",
                "qplatformgraphicsbufferhelper.cpp",
                "qplatformgraphicsbufferhelper.h",
                "qplatforminputcontext.cpp",
                "qplatforminputcontext.h",
                "qplatforminputcontext_p.h",
                "qplatforminputcontextfactory.cpp",
                "qplatforminputcontextfactory_p.h",
                "qplatforminputcontextplugin.cpp",
                "qplatforminputcontextplugin_p.h",
                "qplatformintegration.cpp",
                "qplatformintegration.h",
                "qplatformintegrationfactory.cpp",
                "qplatformintegrationfactory_p.h",
                "qplatformintegrationplugin.cpp",
                "qplatformintegrationplugin.h",
                "qplatformmenu.cpp",
                "qplatformmenu.h",
                "qplatformnativeinterface.cpp",
                "qplatformnativeinterface.h",
                "qplatformoffscreensurface.cpp",
                "qplatformoffscreensurface.h",
                "qplatformscreen.cpp",
                "qplatformscreen.h",
                "qplatformscreen_p.h",
                "qplatformservices.cpp",
                "qplatformservices.h",
                "qplatformsessionmanager.cpp",
                "qplatformsessionmanager.h",
                "qplatformsharedgraphicscache.cpp",
                "qplatformsharedgraphicscache.h",
                "qplatformsurface.cpp",
                "qplatformsurface.h",
                "qplatformsystemtrayicon.cpp",
                "qplatformsystemtrayicon.h",
                "qplatformtheme.cpp",
                "qplatformtheme.h",
                "qplatformtheme_p.h",
                "qplatformthemefactory.cpp",
                "qplatformthemefactory_p.h",
                "qplatformthemeplugin.cpp",
                "qplatformthemeplugin.h",
                "qplatformwindow.cpp",
                "qplatformwindow.h",
                "qplatformwindow_p.h",
                "qrasterwindow.cpp",
                "qrasterwindow.h",
                "qscreen.cpp",
                "qscreen.h",
                "qscreen_p.h",
                "qsessionmanager.cpp",
                "qsessionmanager.h",
                "qsessionmanager_p.h",
                "qshapedpixmapdndwindow.cpp",
                "qshapedpixmapdndwindow_p.h",
                "qshortcutmap.cpp",
                "qshortcutmap_p.h",
                "qsimpledrag.cpp",
                "qsimpledrag_p.h",
                "qstylehints.cpp",
                "qstylehints.h",
                "qsurface.cpp",
                "qsurface.h",
                "qsurfaceformat.cpp",
                "qsurfaceformat.h",
                "qtguiglobal.h",
                "qtguiglobal_p.h",
                "qtouchdevice.cpp",
                "qtouchdevice.h",
                "qtouchdevice_p.h",
                "qwindow.cpp",
                "qwindow.h",
                "qwindow_p.h",
                "qwindowdefs.h",
                "qwindowsysteminterface.cpp",
                "qwindowsysteminterface.h",
                "qwindowsysteminterface_p.h",
            ]
            Group {
                name: "kernel OpenGL"
                condition: QtGuiConfig.opengl
                files: [
                    "qplatformopenglcontext.h",
                    "qopenglcontext.h",
                    "qopenglcontext_p.h",
                    "qopenglwindow.h",
                    "qplatformopenglcontext.cpp",
                    "qopenglcontext.cpp",
                    "qopenglwindow.cpp",
                ]
            }
            Group {
                name: "kernel/Windows"
                condition: qbs.targetOS.contains("windows")
                files: [
                    "qwindowdefs_win.h",
                ]
            }
        }
        Group {
            name: "image"
            prefix: "image/"
            files: [
                "qbitmap.cpp",
                "qbitmap.h",
                "qicon.cpp",
                "qicon.h",
                "qicon_p.h",
                "qiconengine.cpp",
                "qiconengine.h",
                "qiconengineplugin.cpp",
                "qiconengineplugin.h",
                "qiconloader.cpp",
                "qiconloader_p.h",
                "qimage.cpp",
                "qimage.h",
                "qimage_conversions.cpp",
                "qimage_p.h",
                "qimageiohandler.cpp",
                "qimageiohandler.h",
                "qimagepixmapcleanuphooks.cpp",
                "qimagepixmapcleanuphooks_p.h",
                "qimagereader.cpp",
                "qimagereader.h",
                "qimagereaderwriterhelpers.cpp",
                "qimagereaderwriterhelpers_p.h",
                "qimagewriter.cpp",
                "qimagewriter.h",
                "qpaintengine_pic.cpp",
                "qpaintengine_pic_p.h",
                "qpicture.cpp",
                "qpicture.h",
                "qpicture_p.h",
                "qpictureformatplugin.cpp",
                "qpictureformatplugin.h",
                "qpixmap.cpp",
                "qpixmap.h",
                "qpixmap_blitter.cpp",
                "qpixmap_blitter_p.h",
                "qpixmap_raster.cpp",
                "qpixmap_raster_p.h",
                "qpixmapcache.cpp",
                "qpixmapcache.h",
                "qpixmapcache_p.h",
                "qplatformpixmap.cpp",
                "qplatformpixmap.h",
            ]
            Group {
                name: "QMovie"
                condition: QtGuiConfig.movie
                files: [
                    "qmovie.cpp",
                    "qmovie.h",
                ]
            }
            Group {
                name: "image/Desktop Windows"
                condition: qbs.targetOS.contains("windows") && !product.targetsUWP
                files: [
                    "qpixmap_win.cpp",
                ]
            }
            Group {
                name: "image/macOS"
                condition: qbs.targetOS.contains("macos")
                files: [
                    "qimage_darwin.mm",
                ]
            }
            Group {
                name: "image/compat"
                files: [
                    "qimage_compat.cpp",
                ]
                cpp.useCxxPrecompiledHeader: false
            }
            Group {
                name: "Built-in image format support"
                files: [
                    "qbmphandler.cpp",
                    "qbmphandler_p.h",
                    "qppmhandler.cpp",
                    "qppmhandler_p.h",
                    "qxbmhandler.cpp",
                    "qxbmhandler_p.h",
                    "qxpmhandler.cpp",
                    "qxpmhandler_p.h",
                ]
            }
            Group {
                name: "Built-in PNG image format support"
                files: [
                    "qpnghandler.cpp",
                    "qpnghandler_p.h",
                ]
            }
        }
        Depends {
            condition: QtGuiPrivateConfig.png
            name: "qt_libpng"
        }
        Group {
            name: "text"
            prefix: "text/"
            files: [
                "qabstracttextdocumentlayout.cpp",
                "qabstracttextdocumentlayout.h",
                "qabstracttextdocumentlayout_p.h",
                "qdistancefield.cpp",
                "qdistancefield_p.h",
                "qfont.cpp",
                "qfont.h",
                "qfont_p.h",
                "qfontdatabase.cpp",
                "qfontdatabase.h",
                "qfontengine.cpp",
                "qfontengine_p.h",
                "qfontengine_qpf2.cpp",
                "qfontengineglyphcache.cpp",
                "qfontengineglyphcache_p.h",
                "qfontinfo.h",
                "qfontmetrics.cpp",
                "qfontmetrics.h",
                "qfontsubset.cpp",
                "qfontsubset_p.h",
                "qfragmentmap.cpp",
                "qfragmentmap_p.h",
                "qglyphrun.cpp",
                "qglyphrun.h",
                "qglyphrun_p.h",
                "qinputcontrol.cpp",
                "qinputcontrol_p.h",
                "qplatformfontdatabase.cpp",
                "qplatformfontdatabase.h",
                "qrawfont.cpp",
                "qrawfont.h",
                "qrawfont_p.h",
                "qstatictext.cpp",
                "qstatictext.h",
                "qstatictext_p.h",
                "qsyntaxhighlighter.cpp",
                "qsyntaxhighlighter.h",
                "qtextcursor.cpp",
                "qtextcursor.h",
                "qtextcursor_p.h",
                "qtextdocument.cpp",
                "qtextdocument.h",
                "qtextdocument_p.cpp",
                "qtextdocument_p.h",
                "qtextdocumentfragment.cpp",
                "qtextdocumentfragment.h",
                "qtextdocumentfragment_p.h",
                "qtextdocumentlayout.cpp",
                "qtextdocumentlayout_p.h",
                "qtextdocumentwriter.cpp",
                "qtextdocumentwriter.h",
                "qtextengine.cpp",
                "qtextengine_p.h",
                "qtextformat.cpp",
                "qtextformat.h",
                "qtextformat_p.h",
                "qtexthtmlparser.cpp",
                "qtexthtmlparser_p.h",
                "qtextimagehandler.cpp",
                "qtextimagehandler_p.h",
                "qtextlayout.cpp",
                "qtextlayout.h",
                "qtextlist.cpp",
                "qtextlist.h",
                "qtextobject.cpp",
                "qtextobject.h",
                "qtextobject_p.h",
                "qtextoption.cpp",
                "qtextoption.h",
                "qtexttable.cpp",
                "qtexttable.h",
                "qtexttable_p.h",
            ]
            Group {
                condition: QtGuiPrivateConfig.harfbuzz
                name: "text/harfbuzz"
                files: [
                    "qharfbuzzng.cpp",
                    "qharfbuzzng_p.h",
                ]
            }
            Group {
                condition: QtGuiConfig.textodfwriter
                name: "textodfwriter"
                files: [
                    "qtextodfwriter.cpp",
                    "qtextodfwriter_p.h",
                    "qzip.cpp",
                    "qzipreader_p.h",
                    "qzipwriter_p.h",
                ]
            }
        }
        Depends {
            condition: QtGuiPrivateConfig.harfbuzz
            name: "qt_harfbuzz"
        }

        Group {
            condition: QtGuiConfig.cssparser
            name: "ccssparser"
            files: [
                "painting/qcssutil.cpp",
                "text/qcssparser.cpp",
                "text/qcssparser_p.h",
            ]
        }
        Group {
            name: "painting"
            prefix: "painting/"
            files: [
                "qbackingstore.cpp",
                "qbackingstore.h",
                "qbezier.cpp",
                "qbezier_p.h",
                "qblendfunctions.cpp",
                "qblendfunctions_p.h",
                "qblittable.cpp",
                "qblittable_p.h",
                "qbrush.cpp",
                "qbrush.h",
                "qcolor.cpp",
                "qcolor.h",
                "qcolor_p.h",
                "qcolorprofile.cpp",
                "qcolorprofile_p.h",
                "qcompositionfunctions.cpp",
                "qcosmeticstroker.cpp",
                "qcosmeticstroker_p.h",
                "qdatabuffer_p.h",
                "qdrawhelper_p.h",
                "qdrawhelper_x86_p.h",
                "qdrawingprimitive_sse2_p.h",
                "qemulationpaintengine.cpp",
                "qemulationpaintengine_p.h",
                "qfixed_p.h",
                "qgrayraster.c",
                "qgrayraster_p.h",
                "qimagescale.cpp",
                "qmatrix.cpp",
                "qmatrix.h",
                "qmemrotate.cpp",
                "qmemrotate_p.h",
                "qoutlinemapper.cpp",
                "qoutlinemapper_p.h",
                "qpagedpaintdevice.cpp",
                "qpagedpaintdevice.h",
                "qpagedpaintdevice_p.h",
                "qpagelayout.cpp",
                "qpagelayout.h",
                "qpagesize.cpp",
                "qpagesize.h",
                "qpaintdevice.cpp",
                "qpaintdevice.h",
                "qpaintengine.cpp",
                "qpaintengine.h",
                "qpaintengine_blitter.cpp",
                "qpaintengine_blitter_p.h",
                "qpaintengine_p.h",
                "qpaintengine_raster.cpp",
                "qpaintengine_raster_p.h",
                "qpaintengineex.cpp",
                "qpaintengineex_p.h",
                "qpainter.cpp",
                "qpainter.h",
                "qpainter_p.h",
                "qpainterpath.cpp",
                "qpainterpath.h",
                "qpainterpath_p.h",
                "qpathclipper.cpp",
                "qpathclipper_p.h",
                "qpathsimplifier.cpp",
                "qpathsimplifier_p.h",
                "qpdf.cpp",
                "qpdf.qrc",
                "qpdf_p.h",
                "qpdfwriter.cpp",
                "qpdfwriter.h",
                "qpen.cpp",
                "qpen.h",
                "qplatformbackingstore.cpp",
                "qplatformbackingstore.h",
                "qpolygon.cpp",
                "qpolygon.h",
                "qpolygonclipper_p.h",
                "qrasterdefs_p.h",
                "qrasterizer.cpp",
                "qrasterizer_p.h",
                "qrbtree_p.h",
                "qregion.cpp",
                "qregion.h",
                "qrgb.h",
                "qrgba64.h",
                "qrgba64_p.h",
                "qstroker.cpp",
                "qstroker_p.h",
                "qtextureglyphcache.cpp",
                "qtextureglyphcache_p.h",
                "qtransform.cpp",
                "qtransform.h",
                "qtriangulatingstroker.cpp",
                "qtriangulatingstroker_p.h",
                "qtriangulator.cpp",
                "qtriangulator_p.h",
                "qvectorpath_p.h",
            ]
            Group {
                condition: qbs.targetOS.contains("darwin")
                name: "darwin painting support"
                files: [
                    "qcoregraphics.mm",
                    "qcoregraphics_p.h",
                ]
            }
            Group {
                cpp.useCxxPrecompiledHeader: !qbs.toolchain.contains("gcc") || cpp.compilerVersionMajor !== 5
                files: [
                    // Turning on PCH for this file causes internal compiler errors with at least GCC 5.3.1:
                    "qdrawhelper.cpp",
                ]
            }
        }

        Group {
            name: "util"
            prefix: "util/"
            files: [
                "qabstractlayoutstyleinfo.cpp",
                "qabstractlayoutstyleinfo_p.h",
                "qdesktopservices.cpp",
                "qdesktopservices.h",
                "qgridlayoutengine.cpp",
                "qgridlayoutengine_p.h",
                "qhexstring_p.h",
                "qlayoutpolicy.cpp",
                "qlayoutpolicy_p.h",
                "qshaderformat.cpp",
                "qshaderformat_p.h",
                "qshadergenerator.cpp",
                "qshadergenerator_p.h",
                "qshadergraph.cpp",
                "qshadergraph_p.h",
                "qshadergraphloader.cpp",
                "qshadergraphloader_p.h",
                "qshaderlanguage.cpp",
                "qshaderlanguage_p.h",
                "qshadernode.cpp",
                "qshadernode_p.h",
                "qshadernodeport.cpp",
                "qshadernodeport_p.h",
                "qshadernodesloader.cpp",
                "qshadernodesloader_p.h",
                "qvalidator.cpp",
                "qvalidator.h",
            ]
        }

        Group {
            name: "math3d"
            prefix: "math3d/"
            files: [
                "qgenericmatrix.cpp",
                "qgenericmatrix.h",
                "qmatrix4x4.cpp",
                "qmatrix4x4.h",
                "qquaternion.cpp",
                "qquaternion.h",
                "qvector2d.cpp",
                "qvector2d.h",
                "qvector3d.cpp",
                "qvector3d.h",
                "qvector4d.cpp",
                "qvector4d.h",
            ]
        }
        Group {
            condition: QtGuiConfig.standarditemmodel
            name: "itemmodels"
            prefix: "itemmodels/"
            files: [
                "qstandarditemmodel.cpp",
                "qstandarditemmodel.h",
                "qstandarditemmodel_p.h",
            ]
        }
        Group {
            condition: QtGuiConfig.opengl
            name: "OpenGL"
            prefix: "opengl/"
            files: [
                "qopengl.cpp",
                "qopengl.h",
                "qopengl2pexvertexarray.cpp",
                "qopengl2pexvertexarray_p.h",
                "qopengl_p.h",
                "qopenglbuffer.cpp",
                "qopenglbuffer.h",
                "qopenglcustomshaderstage.cpp",
                "qopenglcustomshaderstage_p.h",
                "qopengldebug.cpp",
                "qopengldebug.h",
                "qopenglengineshadermanager.cpp",
                "qopenglengineshadermanager_p.h",
                "qopenglengineshadersource_p.h",
                "qopenglextensions_p.h",
                "qopenglextrafunctions.h",
                "qopenglframebufferobject.cpp",
                "qopenglframebufferobject.h",
                "qopenglframebufferobject_p.h",
                "qopenglfunctions.cpp",
                "qopenglfunctions.h",
                "qopenglgradientcache.cpp",
                "qopenglgradientcache_p.h",
                "qopenglpaintdevice.cpp",
                "qopenglpaintdevice.h",
                "qopenglpaintdevice_p.h",
                "qopenglpaintengine.cpp",
                "qopenglpaintengine_p.h",
                "qopenglpixeltransferoptions.cpp",
                "qopenglpixeltransferoptions.h",
                "qopenglprogrambinarycache.cpp",
                "qopenglprogrambinarycache_p.h",
                "qopenglshadercache_p.h",
                "qopenglshaderprogram.cpp",
                "qopenglshaderprogram.h",
                "qopengltexture.cpp",
                "qopengltexture.h",
                "qopengltexture_p.h",
                "qopengltextureblitter.cpp",
                "qopengltextureblitter.h",
                "qopengltexturecache.cpp",
                "qopengltexturecache_p.h",
                "qopengltextureglyphcache.cpp",
                "qopengltextureglyphcache_p.h",
                "qopengltexturehelper.cpp",
                "qopengltexturehelper_p.h",
                "qopenglversionfunctions.cpp",
                "qopenglversionfunctions.h",
                "qopenglversionfunctionsfactory.cpp",
                "qopenglversionfunctionsfactory_p.h",
                "qopenglvertexarrayobject.cpp",
                "qopenglvertexarrayobject.h",
            ]
            Group {
                condition: !QtGuiConfig.opengles2
                name: "no OpenGL ES"
                files: [
                    "qopenglfunctions_1_0.cpp",
                    "qopenglfunctions_1_0.h",
                    "qopenglfunctions_1_1.cpp",
                    "qopenglfunctions_1_1.h",
                    "qopenglfunctions_1_2.cpp",
                    "qopenglfunctions_1_2.h",
                    "qopenglfunctions_1_3.cpp",
                    "qopenglfunctions_1_3.h",
                    "qopenglfunctions_1_4.cpp",
                    "qopenglfunctions_1_4.h",
                    "qopenglfunctions_1_5.cpp",
                    "qopenglfunctions_1_5.h",
                    "qopenglfunctions_2_0.cpp",
                    "qopenglfunctions_2_0.h",
                    "qopenglfunctions_2_1.cpp",
                    "qopenglfunctions_2_1.h",
                    "qopenglfunctions_3_0.cpp",
                    "qopenglfunctions_3_0.h",
                    "qopenglfunctions_3_1.cpp",
                    "qopenglfunctions_3_1.h",
                    "qopenglfunctions_3_2_compatibility.cpp",
                    "qopenglfunctions_3_2_compatibility.h",
                    "qopenglfunctions_3_2_core.cpp",
                    "qopenglfunctions_3_2_core.h",
                    "qopenglfunctions_3_3_compatibility.cpp",
                    "qopenglfunctions_3_3_compatibility.h",
                    "qopenglfunctions_3_3_core.cpp",
                    "qopenglfunctions_3_3_core.h",
                    "qopenglfunctions_4_0_compatibility.cpp",
                    "qopenglfunctions_4_0_compatibility.h",
                    "qopenglfunctions_4_0_core.cpp",
                    "qopenglfunctions_4_0_core.h",
                    "qopenglfunctions_4_1_compatibility.cpp",
                    "qopenglfunctions_4_1_compatibility.h",
                    "qopenglfunctions_4_1_core.cpp",
                    "qopenglfunctions_4_1_core.h",
                    "qopenglfunctions_4_2_compatibility.cpp",
                    "qopenglfunctions_4_2_compatibility.h",
                    "qopenglfunctions_4_2_core.cpp",
                    "qopenglfunctions_4_2_core.h",
                    "qopenglfunctions_4_3_compatibility.cpp",
                    "qopenglfunctions_4_3_compatibility.h",
                    "qopenglfunctions_4_3_core.cpp",
                    "qopenglfunctions_4_3_core.h",
                    "qopenglfunctions_4_4_compatibility.cpp",
                    "qopenglfunctions_4_4_compatibility.h",
                    "qopenglfunctions_4_4_core.cpp",
                    "qopenglfunctions_4_4_core.h",
                    "qopenglfunctions_4_5_compatibility.cpp",
                    "qopenglfunctions_4_5_compatibility.h",
                    "qopenglfunctions_4_5_core.cpp",
                    "qopenglfunctions_4_5_core.h",
                    "qopenglqueryhelper_p.h",
                    "qopengltimerquery.cpp",
                    "qopengltimerquery.h",
                ]
            }
            Group {
                condition: QtGuiConfig.opengles2
                name: "OpenGL ES"
                files: [
                    "qopenglfunctions_es2.cpp",
                    "qopenglfunctions_es2.h",
                ]
            }
        }
        Group {
            cpp.useCxxPrecompiledHeader: false
            Group {
                condition: Qt.global.privateConfig.avx2
                cpufeatures.x86_avx2: true
                files: [
                    "image/qimage_avx2.cpp",
                    "painting/qdrawhelper_avx2.cpp",
                ]
            }
            Group {
                condition: Qt.global.privateConfig.mips_dspr2
                cpufeatures.mips_dspr2: true
                files: [
                    "image/qimage_mips_dspr2.cpp",
                    "image/qimage_mips_dspr2_asm.S",
                    "painting/qdrawhelper_mips_dspr2_asm.S",
                ]
            }
            Group {
                condition: Qt.global.privateConfig.mips_dsp
                cpufeatures.mips_dsp: true
                files: [
                    "painting/qdrawhelper_mips_dsp.cpp",
                    "painting/qdrawhelper_mips_dsp_asm.S",
                    "painting/qdrawhelper_mips_dsp_p.h",
                    "painting/qt_mips_asm_dsp_p.h",
                ]
            }
            Group {
                condition: Qt.global.privateConfig.neon
                cpufeatures.arm_neon: true
                files: [
                    "../3rdparty/pixman/pixman-arm-neon-asm.S",
                    "image/qimage_neon.cpp",
                    "painting/qdrawhelper_neon.cpp",
                    "painting/qdrawhelper_neon_asm.S",
                    "painting/qdrawhelper_neon_p.h",
                    "painting/qimagescale_neon.cpp",
                ]
            }
            Group {
                condition: Qt.global.privateConfig.sse2
                cpufeatures.x86_sse2: true
                files: [
                    "image/qimage_sse2.cpp",
                    "painting/qdrawhelper_sse2.cpp",
                ]
            }
            Group {
                condition: Qt.global.privateConfig.sse4_1
                cpufeatures.x86_sse4_1: true
                files: [
                    "image/qimage_sse4.cpp",
                    "painting/qdrawhelper_sse4.cpp",
                    "painting/qimagescale_sse4.cpp",
                ]
            }
            Group {
                condition: Qt.global.privateConfig.ssse3
                cpufeatures.x86_ssse3: true
                files: [
                    "image/qimage_ssse3.cpp",
                    "painting/qdrawhelper_ssse3.cpp",
                ]
            }
        }
    }
}

/*

qtConfig(opengl.*): MODULE_CONFIG = opengl

QMAKE_DOCS = $$PWD/doc/qtgui.qdocconf

MODULE_PLUGIN_TYPES = \
    platforms \
    platforms/darwin \
    xcbglintegrations \
    platformthemes \
    platforminputcontexts \
    generic \
    iconengines \
    imageformats \
    egldeviceintegrations

# This is here only because the platform plugin is no module, obviously.
qtConfig(angle) {
    MODULE_AUX_INCLUDES = \
        \$\$QT_MODULE_INCLUDE_BASE/QtANGLE
}

# Code coverage with TestCocoon
# The following is required as extra compilers use $$QMAKE_CXX instead of $(CXX).
# Without this, testcocoon.prf is read only after $$QMAKE_CXX is used by the
# extra compilers.
testcocoon {
    load(testcocoon)
}

CONFIG += simd optimize_full

!uikit:!win32:contains(QT_ARCH, "arm"): CONFIG += no_clang_integrated_as
!uikit:!win32:!contains(QT_ARCH, "arm64"): DEFINES += ENABLE_PIXMAN_DRAWHELPERS

QMAKE_LIBS += $$QMAKE_LIBS_GUI


TRACEPOINT_PROVIDER = $$PWD/qtgui.tracepoints
CONFIG += qt_tracepoints
*/

import qbs
import QtGuiConfig

QtModuleProject {
    name: "QtOpenGL"
    simpleName: "opengl"
    conditionFunction: (function() {
        return QtGuiConfig.opengl;
    })

    QtHeaders {
        Depends { name: "QtWidgetsHeaders" }
    }

    QtPrivateModule {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.widgets" }
            Depends { name: "opengl" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }
        Depends { name: "Qt.gui_private" }
        Depends { name: "Qt.widgets_private" }
        Depends { name: "opengl" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat([
            "QT_NO_FOREACH",
            "QT_NO_USING_NAMESPACE",
        ])

        Properties {
            condition: qbs.toolchain.contains("msvc") && qbs.architecture === "x86"
            cpp.linkerFlags: base.concat("/BASE:0x63000000")
        }

        files: [
            "gl2paintengineex/qgl2pexvertexarray.cpp",
            "gl2paintengineex/qgl2pexvertexarray_p.h",
            "gl2paintengineex/qglcustomshaderstage.cpp",
            "gl2paintengineex/qglcustomshaderstage_p.h",
            "gl2paintengineex/qglengineshadermanager.cpp",
            "gl2paintengineex/qglengineshadermanager_p.h",
            "gl2paintengineex/qglengineshadersource_p.h",
            "gl2paintengineex/qglgradientcache.cpp",
            "gl2paintengineex/qglgradientcache_p.h",
            "gl2paintengineex/qglshadercache_p.h",
            "gl2paintengineex/qpaintengineex_opengl2.cpp",
            "gl2paintengineex/qpaintengineex_opengl2_p.h",
            "gl2paintengineex/qtextureglyphcache_gl.cpp",
            "gl2paintengineex/qtextureglyphcache_gl_p.h",
            "qgl.cpp",
            "qgl.h",
            "qgl_p.h",
            "qglbuffer.cpp",
            "qglbuffer.h",
            "qglcolormap.cpp",
            "qglcolormap.h",
            "qglframebufferobject.cpp",
            "qglframebufferobject.h",
            "qglframebufferobject_p.h",
            "qglfunctions.cpp",
            "qglfunctions.h",
            "qglpaintdevice.cpp",
            "qglpaintdevice_p.h",
            "qglpixelbuffer.cpp",
            "qglpixelbuffer.h",
            "qglpixelbuffer_p.h",
            "qglshaderprogram.cpp",
            "qglshaderprogram.h",
            "qtopenglglobal.h",
        ]

        Group {
            condition: Qt.widgets.config.graphicseffect
            files: [
                "qgraphicsshadereffect.cpp",
                "qgraphicsshadereffect_p.h",
            ]
        }

        Group {
            name: "Qt.core precompiled header"
            files: ["../corelib/global/qt_pch.h"]
        }
    }
}

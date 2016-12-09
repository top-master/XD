import qbs

QtModuleProject {
    id: root
    name: "QtOpenGL"
    simpleName: "opengl"
    prefix: project.qtbasePrefix + "src/opengl/"

    Product {
        name: root.privateName
        condition: project.opengl
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
        condition: project.widgets
        sync.module: root.name
        sync.classNames: ({
            "qgl.h": ["QGL"],
        })
        Depends { name: "QtCoreHeaders" }
        Depends { name: "QtGuiHeaders" }
        Depends { name: "QtWidgetsHeaders" }
        Group {
            name: "headers"
            prefix: project.qtbasePrefix + "src/opengl/"
            files: "**/*.h"
            excludeFiles: "doc/**"
            fileTags: "hpp_syncable"
        }
    }

    QtModule {
        name: root.moduleName
        condition: project.opengl
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "opengl.pro"

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }

            Properties {
                condition: project.opengl && !project.opengles2
                cpp.includePaths: base.concat(root.publicIncludePaths, project.openglIncludeDirs)
                cpp.libraryPaths: base.concat(project.openglLibDirs)
                cpp.dynamicLibraries: base.concat(project.openglLibs)
                cpp.frameworks: base.concat(project.openglFrameworks)
            }
            Properties {
                condition: project.opengles2
                cpp.includePaths: base.concat(root.publicIncludePaths, project.opengles2IncludeDirs)
                cpp.libraryPaths: base.concat(project.opengles2LibDirs)
                cpp.dynamicLibraries: base.concat(project.opengles2Libs)
                cpp.frameworks: base.concat(project.opengles2Frameworks)
            }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends {
            name: "Qt"
            submodules: ["core", "core-private", "gui", "gui-private", "widgets", "widgets-private"]
        }

        cpp.defines: base.concat(["QT_NO_USING_NAMESPACE", "QT_BUILD_OPENGL_LIB"])

        Properties {
            condition: qbs.toolchain.contains("msvc") ||
                       (qbs.targetOS.contains("windows") && qbs.toolchain.contains("icc"))
            cpp.linkerFlags: base.concat("/BASE:0x63000000")
        }

        Properties {
            condition: qbs.targetOS.contains("solaris")
            cpp.optimization: "none"
        }

        Properties {
            condition: qbs.targetOS.contains("irix")
            cpp.cxxFlags: base.concat(["-no_prelink", "-ptused"])
        }

        cpp.includePaths: root.includePaths.concat(base)

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
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
                "qgraphicsshadereffect.cpp",
                "qgraphicsshadereffect_p.h",
                "qtopenglglobal.h",
            ]
        }

        Group {
            name: "paint engine"
            prefix: "gl2paintengineex/"
            files: [
                "qglcustomshaderstage_p.h",
                "qgl2pexvertexarray.cpp",
                "qgl2pexvertexarray_p.h",
                "qglcustomshaderstage.cpp",
                "qglengineshadermanager.cpp",
                "qglengineshadermanager_p.h",
                "qglengineshadersource_p.h",
                "qglgradientcache.cpp",
                "qglgradientcache_p.h",
                "qglshadercache_meego_p.h",
                "qglshadercache_p.h",
                "qpaintengineex_opengl2.cpp",
                "qpaintengineex_opengl2_p.h",
                "qtextureglyphcache_gl.cpp",
                "qtextureglyphcache_gl_p.h",
            ]
        }
    }
}

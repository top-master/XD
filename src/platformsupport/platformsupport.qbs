import qbs
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtPlatformSupport"
    simpleName: "platformsupport"
    prefix: project.qtbasePrefix + "src/platformsupport/"

    QtHeaders {
        name: root.headersName
        sync.module: root.name
        ModuleHeaders { fileTags: "header_sync" }
    }

    QtModule {
        name: root.privateName
        parentName: root.name
        simpleName: root.simpleName + "_private"
        targetName: root.targetName
        qmakeProject: root.prefix + "platformsupport.pro"
        type: ["staticlibrary", "prl", "pri"]

        Export {
            Depends { name: "cpp" }
            cpp.defines: root.defines
            cpp.includePaths: root.includePaths
        }

        cpp.includePaths: base.concat(root.includePaths)

        Depends { name: root.headersName }
        Depends { name: "egl"; condition: project.egl }
        Depends { name: "freetype2" }
        Depends { name: "glib"; condition: project.glib }
        Depends { name: "Qt"; submodules: ["core", "core-private", "gui", "gui-private", "platformheaders"] }
        Depends { name: "Qt.dbus"; condition: project.dbus; }

        Group {
            name: "precompiled header from corelib"
            files: [project.corelibPrecompiledHeader]
        }
        Properties {
            condition: project.precompiledHeaders
            cpp.cxxPrecompiledHeader: project.corelibPrecompiledHeader
        }

        Group {
            name: "sources_eglconvenience"
            condition: project.egl
            prefix: root.prefix + "eglconvenience/"
            cpp.defines: outer.concat("MESA_EGL_NO_X11_HEADERS")
            files: [
                "qeglconvenience.cpp",
                "qeglpbuffer.cpp",
                "qeglplatformcontext.cpp",
            ]
        }

        Group {
            name: "sources_eglconvenience_x11"
            condition: project.egl && project.x11
            prefix: root.prefix + "eglconvenience/"
            files: [
                "qxlibeglintegration.cpp",
            ]
        }

        Group {
            name: "sources_eventdispatchers_glib"
            condition: project.glib
            prefix: root.prefix + "eventdispatchers/"
            files: [
                "qeventdispatcher_glib_p.h",
                "qeventdispatcher_glib.cpp",
            ]
        }

        Group {
            name: "sources_eventdispatchers_unix"
            condition: qbs.targetOS.contains("unix")
            prefix: root.prefix + "eventdispatchers/"
            files: [
                "qunixeventdispatcher.cpp",
                "qgenericunixeventdispatcher.cpp",
                "qunixeventdispatcher_qpa_p.h",
                "qgenericunixeventdispatcher_p.h",
            ]
        }

        Group {
            name: "sources_themes_genericunix"
            condition: qbs.targetOS.contains("unix")
            prefix: root.prefix + "themes/genericunix/"
            files: [
                "qgenericunixthemes.cpp",
                "qgenericunixthemes_p.h",
            ]
        }

        Group {
            name: "sources_services_genericunix"
            condition: qbs.targetOS.contains("unix")
            prefix: root.prefix + "services/genericunix/"
            files: [
                "qgenericunixservices.cpp",
                "qgenericunixservices_p.h",
            ]
        }

        Group {
            name: "sources_fontdatabases_basic"
            prefix: root.prefix + "fontdatabases/basic/"
            files: [
                "qbasicfontdatabase.cpp",
            ]
        }

        Group {
            name: "sources_fontdatabases_basic_ft"
            prefix: project.qtbasePrefix + "src/gui/text/"
            files: [
                "qfontengine_ft.cpp",
            ]
        }
    }
}

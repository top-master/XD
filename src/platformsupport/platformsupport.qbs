import qbs
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtPlatformSupport"
    moduleName: "Qt.platformsupport-private"
    simpleName: "platformsupport"
    prefix: project.qtbasePrefix + "src/platformsupport/"

    QtHeaders {
        name: root.headersName
        sync.module: root.name
        Depends { name: "QtCoreHeaders" }
        Depends { name: "QtGuiHeaders" }
        ModuleHeaders { fileTags: "hpp_syncable" }
    }

    QtModule {
        name: root.moduleName
        parentName: root.name
        simpleName: root.simpleName + "_private"
        targetName: root.targetName
        qmakeProject: root.prefix + "platformsupport.pro"
        type: ["staticlibrary", "prl", "pri"]

        Export {
            Depends { name: "cpp" }
            cpp.defines: root.defines
            cpp.includePaths: root.includePaths

            Properties {
                condition: qbs.targetOS.contains("darwin")
                cpp.frameworks: {
                    var frameworks = ["CoreGraphics", "CoreText"];
                    if (qbs.targetOS.contains("osx"))
                        frameworks.push("AppKit", "OpenGL");
                    return frameworks;
                }
            }
        }

        cpp.useCxxPrecompiledHeader: project.precompiledHeaders
        cpp.includePaths: [path].concat(base).concat(root.includePaths)

        Depends { name: root.headersName }
        Depends { name: "egl"; condition: project.egl }
        Depends { name: "freetype2" }
        Depends { name: "glib"; condition: project.glib }
        Depends { name: "Qt"; submodules: ["core", "core-private", "gui", "gui-private", "platformheaders"] }
        Depends { name: "Qt.dbus"; condition: project.dbus }

        Group {
            name: "precompiled header from corelib"
            files: [project.corelibPrecompiledHeader]
        }

        Group {
            name: "sources_accessibility"
            condition: project.accessibility
            prefix: root.prefix + "accessibility/"
            files: [
                "qaccessiblebridgeutils.cpp",
                "qaccessiblebridgeutils_p.h",
            ]
        }

        Group {
            name: "sources_cglconvenience"
            condition: qbs.targetOS.contains("darwin")
            prefix: root.prefix + "cglconvenience/"
            files: [
                "cglconvenience.mm",
                "cglconvenience_p.h",
            ]
        }

        Group {
            name: "sources_clipboard"
            condition: qbs.targetOS.contains("darwin")
            prefix: root.prefix + "clipboard/"
            files: [
                "qmacmime.mm",
                "qmacmime_p.h",
            ]
        }

        Group {
            name: "sources_dbusmenu"
            condition: project.dbus && qbs.targetOS.contains("linux")
            prefix: root.prefix + "dbusmenu/"
            files: [
                "qdbusmenuadaptor.cpp",
                "qdbusmenuadaptor_p.h",
                "qdbusmenubar.cpp",
                "qdbusmenubar_p.h",
                "qdbusmenuconnection.cpp",
                "qdbusmenuconnection_p.h",
                "qdbusmenuregistrarproxy.cpp",
                "qdbusmenuregistrarproxy_p.h",
                "qdbusmenutypes.cpp",
                "qdbusmenutypes_p.h",
                "qdbusplatformmenu.cpp",
                "qdbusplatformmenu_p.h",
            ]
        }

        Group {
            name: "sources_dbustray"
            condition: project.dbus && qbs.targetOS.contains("linux")
            prefix: root.prefix + "dbustray/"
            files: [
                "qdbustrayicon_p.h",
                "qdbustraytypes_p.h",
                "qstatusnotifieritemadaptor_p.h",
                "qxdgnotificationproxy_p.h",
                "qdbustrayicon.cpp",
                "qdbustraytypes.cpp",
                "qstatusnotifieritemadaptor.cpp",
                "qxdgnotificationproxy.cpp",
            ]
        }

        Group {
            name: "sources_eglconvenience"
            condition: project.egl && project.opengl
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
            name: "sources_fontdatabases_mac"
            condition: qbs.targetOS.contains("darwin")
            prefix: root.prefix + "fontdatabases/mac/"
            files: [
                "qcoretextfontdatabase.mm",
                "qcoretextfontdatabase_p.h",
                "qfontengine_coretext.mm",
                "qfontengine_coretext_p.h",
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

import qbs
import "headers.qbs" as ModuleHeaders

Project {
    references: [
        "glxconvenience",
    ]

    QtModuleProject {
        id: root
        name: "QtPlatformSupport"
        moduleName: "Qt.platformsupport-private"
        simpleName: "platformsupport"
        prefix: project.qtbasePrefix + "src/platformsupport/"

        defines: {
            var defines = [];
            if (!project.evdev)
                defines.push("QT_NO_EVDEV");
            if (!project.libinput)
                defines.push("QT_NO_LIBINPUT");
            if (!project.mtdev)
                defines.push("QT_NO_MTDEV");
            if (!project.tslib)
                defines.push("QT_NO_TSLIB");
            if (!project.udev)
                defines.push("QT_NO_UDEV");
            return defines.concat(base);
        }

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
                Depends { name: "fontconfig"; condition: project.config.contains("fontconfig") }
                Depends { name: "freetype2" }
                Depends { name: "glib"; condition: project.glib }
                Depends { name: "libinput"; condition: project.libinput }
                Depends { name: "libudev"; condition: project.libudev }
                Depends { name: "mtdev"; condition: project.mtdev }
                Depends { name: "tslib"; condition: project.tslib }
                Depends { name: "xkbcommon"; condition: project.xkbcommon_evdev }
                Depends { name: "cpp" }
                Depends { name: "Qt.dbus"; condition: project.dbus }
                Depends { name: "x11"; condition: project.egl && project.config.contains("xlib") }
                cpp.defines: {
                    var defines = root.defines;
                    if (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
                            && project.config.contains("fontconfig")) {
                        defines.push("Q_FONTCONFIGDATABASE");
                    }
                    return defines;
                }
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

            cpp.defines: root.defines
            cpp.useCxxPrecompiledHeader: project.precompiledHeaders
            cpp.includePaths: [path].concat(base).concat(root.includePaths)

            Depends { name: root.headersName }
            Depends { name: "egl"; condition: project.egl }
            Depends { name: "freetype2" }
            Depends { name: "glib"; condition: project.glib }
            Depends { name: "libinput"; condition: project.libinput }
            Depends { name: "mtdev"; condition: project.mtdev }
            Depends { name: "libudev"; condition: project.libudev }
            Depends { name: "tslib"; condition: project.tslib }
            Depends { name: "xkbcommon"; condition: project.xkbcommon_evdev }
            Depends { name: "x11"; condition: project.egl && project.config.contains("xlib") }
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
                name: "sources (device discovery)"
                condition: qbs.targetOS.contains("linux")
                prefix: root.prefix + "devicediscovery/"
                files: [
                    "qdevicediscovery_dummy.cpp",
                    "qdevicediscovery_dummy_p.h",
                    "qdevicediscovery_p.h",
                    "qdevicediscovery_static.cpp",
                    "qdevicediscovery_static_p.h",
                    "qdevicediscovery_udev.cpp",
                    "qdevicediscovery_udev_p.h",
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
                condition: project.egl && project.config.contains("xlib")
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
                name: "sources (fb convenience)"
                condition: project.opengl
                prefix: root.prefix + "fbconvenience/"
                files: [
                    "qfbbackingstore.cpp",
                    "qfbbackingstore_p.h",
                    "qfbcursor.cpp",
                    "qfbcursor_p.h",
                    "qfbscreen.cpp",
                    "qfbscreen_p.h",
                    "qfbvthandler.cpp",
                    "qfbvthandler_p.h",
                    "qfbwindow.cpp",
                    "qfbwindow_p.h",
                ]
            }

            Group {
                name: "sources (evdev keyboard)"
                condition: project.evdev
                prefix: root.prefix + "input/evdevkeyboard/"
                files: [
                    "qevdevkeyboard_defaultmap_p.h",
                    "qevdevkeyboardhandler.cpp",
                    "qevdevkeyboardhandler_p.h",
                    "qevdevkeyboardmanager.cpp",
                    "qevdevkeyboardmanager_p.h",
                ]
            }

            Group {
                name: "sources (evdev mouse)"
                condition: project.evdev
                prefix: root.prefix + "input/evdevmouse/"
                files: [
                    "qevdevmousehandler.cpp",
                    "qevdevmousehandler_p.h",
                    "qevdevmousemanager.cpp",
                    "qevdevmousemanager_p.h",
                ]
            }

            Group {
                name: "sources (evdev tablet)"
                condition: project.evdev
                prefix: root.prefix + "input/evdevtablet/"
                files: [
                    "qevdevtablethandler.cpp",
                    "qevdevtablethandler_p.h",
                    "qevdevtabletmanager.cpp",
                    "qevdevtabletmanager_p.h",
                ]
            }

            Group {
                name: "sources (evdev touch)"
                condition: project.evdev
                prefix: root.prefix + "input/evdevtouch/"
                files: [
                    "qevdevtouchhandler.cpp",
                    "qevdevtouchhandler_p.h",
                    "qevdevtouchmanager.cpp",
                    "qevdevtouchmanager_p.h",
                ]
            }

            Group {
                name: "sources (evdev libinput)"
                condition: project.libinput
                prefix: root.prefix + "input/libinput/"
                files: [
                    "qlibinputhandler.cpp",
                    "qlibinputhandler_p.h",
                    "qlibinputkeyboard.cpp",
                    "qlibinputkeyboard_p.h",
                    "qlibinputpointer.cpp",
                    "qlibinputpointer_p.h",
                    "qlibinputtouch.cpp",
                    "qlibinputtouch_p.h",
                ]
            }

            Group {
                name: "sources (tslib)"
                condition: project.tslib
                prefix: root.prefix + "input/tslib/"
                files: [
                    "qtslib.cpp",
                    "qtslib_p.h",
                ]
            }

            Group {
                name: "sources (platform compositor)"
                condition: project.opengl
                prefix: root.prefix + "platformcompositor/"
                files: [
                    "qopenglcompositorbackingstore.cpp",
                    "qopenglcompositorbackingstore_p.h",
                    "qopenglcompositor.cpp",
                    "qopenglcompositor_p.h",
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

            Group {
                name: "sources_fontconfig"
                condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
                           && project.config.contains("fontconfig")
                prefix: "fontdatabases/fontconfig/"
                files: [
                    "qfontconfigdatabase.cpp",
                    "qfontenginemultifontconfig.cpp",
                ]
            }
        }
    }
}

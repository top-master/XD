import qbs

QtModuleProject {
    name: "QtFontDatabaseSupport"
    simpleName: "fontdatabase_support"
    internal: true
    conditionFunction: (function() {
        return QtGuiPrivateConfig.freetype || qbs.targetOS.contains("windows") || qbs.targetOS.contains("darwin");
    })
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

    QtHeaders {
        Depends { name: "QtGuiPrivateConfig" }
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core-private" }
        Depends { name: "Qt.gui-private" }
        Depends { name: "QtGuiPrivateConfig" }
        Depends { name: "qt_freetype" }
        Depends { name: "Fontconfig"; condition: QtGuiPrivateConfig.fontconfig }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: {
            var result = base.concat("QT_NO_CAST_FROM_ASCII");
            if (qbs.targetOS.contains("windows")) {
                if (product.targetsUWP)
                    result.push("__WRL_NO_DEFAULT_LIB__");
                else if (QtGuiPrivateConfig.directwrite2)
                    result.push("QT_USE_DIRECTWRITE2");
                else if (QtGuiPrivateConfig.directwrite)
                    result.push("QT_USE_DIRECTWRITE");
                else
                    result.push("QT_NO_DIRECTWRITE");
            }
            return result;
        }
        cpp.dynamicLibraries: {
            var result = [];
            if (qbs.targetOS.contains("windows")) {
                if (product.targetsUWP)
                    result.push("dwrite");
                else
                    result.push("ole32", "gdi32", "user32", "advapi32");
                if (qbs.toolchain.contains("mingw"))
                    result.push("uuid");
            }
            return result;
        }
        cpp.frameworks: {
            var result = [];
            if (qbs.targetOS.contains("darwin")) {
                result.push("CoreFoundation", "CoreGraphics", "CoreText", "Foundation");
                if (qbs.targetOS.contains("macos"))
                    result.push("AppKit", "UIKit");
            }
            return result;
        }

        Group {
            condition: qbs.targetOS.contains("darwin")
            prefix: "mac/"
            files: [
                "qcoretextfontdatabase_p.h",
                "qfontengine_coretext_p.h",
                "qfontengine_coretext.mm",
                "qcoretextfontdatabase.mm",
            ]
        }
        Group {
            condition: QtGuiPrivateConfig.freetype
            prefix: "freetype/"
            files: [
                "qfontengine_ft_p.h",
                "qfontengine_ft.cpp",
            ]
            Group {
                condition: !qbs.targetOS.contains("darwin")
                files: [
                    "qfreetypefontdatabase.cpp",
                    "qfreetypefontdatabase_p.h",
                ]
            }
        }
        Group {
            condition: !qbs.targetOS.contains("darwin") && qbs.targetOS.contains("unix")
            prefix: "genericunix/"
            files: [
                "qgenericunixfontdatabase_p.h",
            ]
            Group {
                condition: QtGuiPrivateConfig.fontconfig
                prefix: "fontconfig/"
                files: [
                    "qfontconfigdatabase.cpp",
                    "qfontconfigdatabase_p.h",
                    "qfontenginemultifontconfig.cpp",
                    "qfontenginemultifontconfig_p.h",
                ]
            }
        }
        Group {
            condition: qbs.targetOS.contains("windows") && !product.targetsUWP
            prefix: "windows/"
            files: [
                "qwindowsfontdatabase.cpp",
                "qwindowsfontdatabase_p.h",
                "qwindowsfontengine.cpp",
                "qwindowsfontengine_p.h",
                "qwindowsnativeimage.cpp",
                "qwindowsnativeimage_p.h",
            ]
            Group {
                condition: QtGuiPrivateConfig.freetype
                files: [
                    "qwindowsfontdatabase_ft.cpp",
                    "qwindowsfontdatabase_ft_p.h",
                ]
            }
            Group {
                condition: QtGuiPrivateConfig.directwrite
                files: [
                    "qwindowsfontenginedirectwrite.cpp",
                    "qwindowsfontenginedirectwrite_p.h",
                ]
            }
        }
        Group {
            condition: product.targetsUWP
            files: [
                "qwinrtfontdatabase.cpp",
                "qwinrtfontdatabase_p.h",
            ]
        }
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}

/*
mac/coretext.pri:

# CoreText is documented to be available on watchOS, but the headers aren't present
# in the watchOS Simulator SDK like they are supposed to be. Work around the problem
# by adding the device SDK's headers to the search path as a fallback.
# rdar://25314492, rdar://27844864
watchos:simulator {
    simulator_system_frameworks = $$xcodeSDKInfo(Path, $${simulator.sdk})/System/Library/Frameworks
    device_system_frameworks = $$xcodeSDKInfo(Path, $${device.sdk})/System/Library/Frameworks
    for (arch, QMAKE_APPLE_SIMULATOR_ARCHS) {
        QMAKE_CXXFLAGS += \
            -Xarch_$${arch} \
            -F$$simulator_system_frameworks \
            -Xarch_$${arch} \
            -F$$device_system_frameworks
    }
}
*/

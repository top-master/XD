import qbs
import qbs.Process
import qbs.TextFile

import QtCoreConfig
import QtCorePrivateConfig
import "animation/animation.qbs" as SrcAnimation
import "codecs/codecs.qbs" as SrcCodecs
import "global/global.qbs" as SrcGlobal
import "io/io.qbs" as SrcIo
import "itemmodels/itemmodels.qbs" as SrcItemModels
import "json/json.qbs" as SrcJson
import "kernel/kernel.qbs" as SrcKernel
import "mimetypes/mimetypes.qbs" as SrcMimeTypes
import "plugin/plugin.qbs" as SrcPlugin
import "statemachine/statemachine.qbs" as SrcStateMachine
import "thread/thread.qbs" as SrcThread
import "tools/tools.qbs" as SrcTools
import "xml/xml.qbs" as SrcXml

QtModuleProject {
    name: "QtCore"
    simpleName: "core"

    property stringList generatedHeaders: [
        project.configPath + "/qconfig.h",
        project.configPath + "/qconfig_p.h",
        project.configPath + "/qconfig.cpp",
        project.qtbaseShadowDir + "/src/corelib/qtcore-config.h",
        project.qtbaseShadowDir + "/src/corelib/qtcore-config_p.h",
    ]

    QtHeaders {
        sync.classNames: ({
            "qglobal.h": ["QtGlobal"],
            "qendian.h": ["QtEndian"],
            "qconfig.h": ["QtConfig"],
            "qplugin.h": ["QtPlugin"],
            "qalgorithms.h": ["QtAlgorithms"],
            "qcontainerfwd.h": ["QtContainerFwd"],
            "qdebug.h": ["QtDebug"],
            "qnamespace.h": ["Qt"],
            "qnumeric.h": ["QtNumeric"],
            "qvariant.h": ["QVariantHash", "QVariantList", "QVariantMap"],
            "qbytearray.h": ["QByteArrayData"],
            "qbytearraylist.h": ["QByteArrayList"],
        })
        shadowBuildFiles: project.generatedHeaders
    }

    QtPrivateModule {
        Export {
            property var config: QtCorePrivateConfig
        }
    }

    QtModule {
        property var config: QtCoreConfig
        property var privateConfig: QtCorePrivateConfig
        Export {
            property var config: QtCoreConfig
            Depends { name: "cpp" }
            cpp.includePaths: project.publicIncludePaths.concat(generatedHeadersDir)

            Depends { name: "moc" }
            Depends { name: "rcc" }

            Depends { name: product.mkspecModule; condition: product.mkspecModule !== undefined }

            property stringList generatedHeadersDir: importingProduct.buildDirectory + "/qt.headers"

            /*
              TODO: Make this work
            Depends {
                name: "qtmain"
                condition: qbs.targetOS.contains("windows")
                           && !importingProduct.consoleApplication
                           && importingProduct.type && importingProduct.type.contains("application")
            }
            */
        }

        Depends { name: project.headersName }

        Depends { name: "moc" }
        Depends { name: "rcc" }

        Depends { name: "Glib"; condition: QtCorePrivateConfig.glib }
        Depends { name: "Iconv"; condition: QtCorePrivateConfig.gnu_libiconv }
        Depends { name: "Journald"; condition: QtCorePrivateConfig.journald }
        Depends { name: "Libdl"; condition: QtCorePrivateConfig.dlopen }
        Depends {
            name: "Pps"
            condition: qbs.targetOS.contains("qnx") && QtCorePrivateConfig.qqnx_pps
        }
        Depends { name: "qt_pcre2"; condition: QtCoreConfig.regularexpression }
        Depends { name: "qt_zlib" }
        qt_zlib.useQtCore: false

        Depends { name: "qt_doubleconversion"; condition: QtCorePrivateConfig.doubleconversion }

        cpp.cxxFlags: {
            var flags = base;
            if (qbs.targetOS.contains("integrity"))
                flags.push("--pending_instantiations=128"); // For QMetaType::typeName
            if (qbs.toolchain.contains("icc"))
                flags.push("-fp-model", "strict");
            return flags;
        }
        cpp.defines: {
            var defines = [];
            if (QtCorePrivateConfig.icu)
                defines.push("QT_USE_ICU");
            if (qbs.targetOS.contains("unix")) {
                if (QtCorePrivateConfig.poll_poll)
                    defines.push("QT_HAVE_POLL");
                if (QtCorePrivateConfig.poll_ppoll)
                    defines.push("QT_HAVE_PPOLL");
                if (QtCorePrivateConfig.poll_pollts)
                    defines.push("QT_HAVE_POLL", "QT_HAVE_POLLTS");
            }
            if (elfInterpreterProbe.found)
                defines.push('ELF_INTERPRETER="' + elfInterpreterProbe.interpreter + '"');
            return defines.concat(base);
        }
        cpp.dynamicLibraries: {
            var dynamicLibraries = base;
            if (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("android")) {
                dynamicLibraries.push("pthread");
                if (QtCorePrivateConfig.clock_gettime
                       && (qbs.targetOS.containsAny(["linux", "hpux", "solaris"]))) {
                    dynamicLibraries.push("rt");
                }
            }
            if (qbs.targetOS.contains("windows")) {
                dynamicLibraries.push("shell32");
                dynamicLibraries.push("user32");
                if (product.targetsUWP) {
                    dynamicLibraries.push("runtimeobject");
                } else {
                    dynamicLibraries.push("advapi32");
                    dynamicLibraries.push("ws2_32");
                    dynamicLibraries.push("mpr");
                    dynamicLibraries.push("uuid");
                    dynamicLibraries.push("ole32");
                    dynamicLibraries.push("winmm");
                    dynamicLibraries.push("version");
                }
            }
            if (QtCorePrivateConfig.icu) {
                if (qbs.targetOS.contains("windows")) {
                    if (Qt.global.config.staticBuild) {
                        if (qbs.enableDebugCode) {
                            dynamicLibraries.push("sicuind", "sicuucd", "sicudtd");
                        } else {
                            dynamicLibraries.push("sicuin", "sicuuc", "sicudt");
                        }
                    } else {
                        dynamicLibraries.push("icuin", "icuuc", "icudt");

                    }
                } else {
                    dynamicLibraries.push("icui18n", "icuuc", "icudata");
                }
            }
            if (QtCorePrivateConfig.slog2)
                dynamicLibraries.push("slog2");
            if (qbs.targetOS.contains("bsd") && !qbs.targetOS.contains("darwin"))
                dynamicLibraries.push("execinfo");
            return dynamicLibraries;
        }
        cpp.frameworks: {
            var frameworks = base;
            if (qbs.targetOS.contains("darwin")) {
                frameworks.push("CoreFoundation", "Foundation");
                if (qbs.targetOS.contains("macos")) {
                    frameworks.push("AppKit", "CoreServices", "DiskArbitration", "IOKit");
                } else {
                    frameworks.push("MobileCoreServices");
                    if (qbs.targetOS.containsAny(["ios", "tvos"]))
                        frameworks.push("UIKit");
                }
            }
            return frameworks;
        }
        cpp.includePaths: project.includePaths.concat([".", "../3rdparty/harfbuzz/src"]).concat(base)

        Properties {
            condition: qbs.toolchain.contains("mingw")
            cpp.minimumWindowsVersion: "6.0"
        }

        cpp.linkerFlags: base.concat(elfInterpreterProbe.found ? ["-e", "qt_core_boilerplate"] : [])

        Probe {
            id: elfInterpreterProbe
            condition: (qbs.targetOS.contains("linux") || qbs.targetOS.contains("hurd"))
                && !Qt.global.config.cross_compile
                && !Qt.global.config.staticBuild // TODO: :!*-armcc
            property string binutilsPrefix: cpp.binutilsPathPrefix
            property string interpreter
            configure: {
                var process = new Process();
                process.setEnv("LC_ALL", "C");
                process.start(binutilsPrefix + "readelf", ["-l", "/bin/ls"]);
                if (!process.waitForFinished() || process.exitCode() !== 0)
                    return;
                var re = /program interpreter: (.*)]/;
                var result = re.exec(process.readStdOut());
                if (!result || result.length < 2)
                    return;
                interpreter = result[1];
                found = true;
            }
        }

        SrcAnimation { }
        SrcCodecs { }
        SrcGlobal { }
        SrcIo { }
        SrcItemModels { }
        SrcJson { }
        SrcKernel { }
        SrcMimeTypes { }
        SrcPlugin { }
        SrcStateMachine { }
        SrcThread { }
        SrcTools { }
        SrcXml { }

        Group {
            name: "sources_harfbuzz"
            prefix: "../3rdparty/harfbuzz/src/"
            files: [
                "harfbuzz-buffer.c",
                "harfbuzz-gdef.c",
                "harfbuzz-gpos.c",
                "harfbuzz-gsub.c",
                "harfbuzz-impl.c",
                "harfbuzz-open.c",
                "harfbuzz-shaper-all.cpp",
                "harfbuzz-stream.c",
                "harfbuzz.h",
            ]
        }

        // TODO: qfloat16-tables stuff

        Group {
            name: "mkspecs"
            condition: !multiplexed
            files: project.qtbaseDir + "/mkspecs/**/*"
            fileTags: []
            qbs.install: true
            qbs.installDir: "mkspecs"
            qbs.installSourceBase: project.qtbaseDir + "/mkspecs"
        }

        Group {
            name: "header files generated by configure"
            files: project.generatedHeaders
        }
    }
}

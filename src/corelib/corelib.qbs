import qbs
import qbs.FileInfo
import qbs.Process
import qbs.TextFile
import qbs.Xml

import QtCoreConfig
import QtCorePrivateConfig
import "animation/animation.qbs" as SrcAnimation
import "codecs/codecs.qbs" as SrcCodecs
import "global/global.qbs" as SrcGlobal
import "io/io.qbs" as SrcIo
import "itemmodels/itemmodels.qbs" as SrcItemModels
import "kernel/kernel.qbs" as SrcKernel
import "mimetypes/mimetypes.qbs" as SrcMimeTypes
import "plugin/plugin.qbs" as SrcPlugin
import "serialization/serialization.qbs" as SrcSerialization
import "statemachine/statemachine.qbs" as SrcStateMachine
import "thread/thread.qbs" as SrcThread
import "tools/tools.qbs" as SrcTools

QtModuleProject {
    name: "QtCore"
    simpleName: "core"
    config: QtCoreConfig
    privateConfig: QtCorePrivateConfig

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
    }

    QtModuleTracepoints {}

    QtModule {
        Properties {
            condition: createPkgconfigFiles
            Exporter.pkgconfig.transformFunction: (function (product, moduleName, propertyName, value) {
                if (product.name === "moc")
                    return undefined;
                return value;
            })
        }

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: project.publicIncludePaths.concat(generatedHeadersDir)

            Depends { name: "moc" }
            Depends { name: "rcc" }

            Depends { name: product.mkspecModule; condition: product.mkspecModule !== undefined }

            property stringList generatedHeadersDir: importingProduct.buildDirectory + "/qt.headers"

            property path resourceSourceBase
            property string resourcePrefix: "/"
            property string resourceFileBaseName: importingProduct.targetName
            Rule {
                multiplex: true
                inputs: ["qt.core.resource_data"]
                Artifact {
                    filePath: product.Qt.core.resourceFileBaseName + ".qrc"
                    fileTags: ["qrc"]
                }
                prepare: {
                    var cmd = new JavaScriptCommand();
                    cmd.description = "generating " + output.fileName;
                    cmd.sourceCode = function() {
                        var doc = new Xml.DomDocument("RCC");

                        var rccNode = doc.createElement("RCC");
                        rccNode.setAttribute("version", "1.0");
                        doc.appendChild(rccNode);

                        var inputsByPrefix = {}
                        for (var i = 0; i < inputs["qt.core.resource_data"].length; ++i) {
                            var inp = inputs["qt.core.resource_data"][i];
                            var prefix = inp.Qt.core.resourcePrefix;
                            var inputsList = inputsByPrefix[prefix] || [];
                            inputsList.push(inp);
                            inputsByPrefix[prefix] = inputsList;
                        }

                        for (var prefix in inputsByPrefix) {
                            var qresourceNode = doc.createElement("qresource");
                            qresourceNode.setAttribute("prefix", prefix);
                            rccNode.appendChild(qresourceNode);

                            for (var i = 0; i < inputsByPrefix[prefix].length; ++i) {
                                var inp = inputsByPrefix[prefix][i];
                                var fullResPath = inp.filePath;
                                var baseDir = inp.Qt.core.resourceSourceBase;
                                var resAlias = baseDir
                                    ? FileInfo.relativePath(baseDir, fullResPath) : inp.fileName;

                                var fileNode = doc.createElement("file");
                                fileNode.setAttribute("alias", resAlias);
                                qresourceNode.appendChild(fileNode);

                                var fileTextNode = doc.createTextNode(fullResPath);
                                fileNode.appendChild(fileTextNode);
                            }
                        }

                        doc.save(output.filePath, 4);
                    };
                    return [cmd];
                }
            }

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
        Depends { name: "qfloat16-tables" }

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
            return defines.concat(base.filter(function(name) {
                return name !== "QT_NO_CAST_TO_ASCII";
            }));
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
                    dynamicLibraries.push("mpr");
                    dynamicLibraries.push("netapi32");
                    dynamicLibraries.push("ole32");
                    dynamicLibraries.push("userenv");
                    dynamicLibraries.push("uuid");
                    dynamicLibraries.push("version");
                    dynamicLibraries.push("winmm");
                    dynamicLibraries.push("ws2_32");
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
        cpp.includePaths: project.includePaths.concat(
            ".",
            "../3rdparty/harfbuzz/src",
            "../3rdparty/tinycbor/src"
        ).concat(base)

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
        SrcKernel { }
        SrcMimeTypes { }
        SrcPlugin { }
        SrcSerialization { }
        SrcStateMachine { }
        SrcThread { }
        SrcTools { }

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

        Rule {
            multiplex: true
            explicitlyDependsOnFromDependencies: "qt.qfloat16-tables-tool"
            Artifact {
                filePath: "qfloat16tables.cpp"
                fileTags: "cpp"
            }
            prepare: {
                var cmd = new Command(explicitlyDependsOn["qt.qfloat16-tables-tool"][0].filePath,
                                      output.filePath);
                cmd.description = "generating " + output.fileName;
                return cmd;
            }
        }
    }
}

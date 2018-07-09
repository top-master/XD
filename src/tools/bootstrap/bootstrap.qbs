import qbs
import qbs.File
import qbs.FileInfo
import QtUtils

QtModuleProject {
    name: "QtBootstrap"
    simpleName: "bootstrap"
    internal: true
    prefix: project.qtbaseDir + "/src/corelib/"
    includePaths: QtUtils.includesForModule("QtCore-private", project.buildDirectory + '/' + simpleName, project.version)
    .concat(QtUtils.includesForModule("QtXml-private", project.buildDirectory + '/' + simpleName, project.version))
    .concat(project.qtbaseShadowDir + "/include")
    .concat(project.qtbaseShadowDir + "/include/QtCore")

    QtModule {
        hostBuild: true
        simpleName: parent.simpleName + "_private"
        type: base.concat("prl", "pri")

        Properties {
            condition: qbs.targetOS.contains("darwin")
            bundle.isBundle: false
        }

        /* TODO:
           CONFIG += gc_binaries
           MODULE_CONFIG = gc_binaries
         */
/* TODO. Possibly move to more generic place
    QMAKE_CFLAGS += $$QMAKE_CFLAGS_SPLIT_SECTIONS
    QMAKE_CXXFLAGS += $$QMAKE_CXXFLAGS_SPLIT_SECTIONS
*/

        // TODO: Move to more generic place? Also needed by bootstrap-dbus and potentially others.
        Properties {
            condition: Qt.global.config.release_tools
            qbs.buildVariant: "release"
        }
        qbs.buildVariant: original

        Properties {
            condition: createPkgconfigFiles
            Exporter.pkgconfig.excludedDependencies: ["Qt.core"]
        }

        // Ignore debug_and_release for host tools.
        multiplexByQbsProperties: base.filter(function(name) { return name !== "buildVariants"; })

        Depends { name: "QtCoreHeaders" }
        Depends { name: "QtXmlHeaders" }
        Depends { name: "cpp" }
        Depends { name: "qt_zlib" }

        cpp.includePaths: project.includePaths.concat(
            QtCoreHeaders.includePaths,
            QtXmlHeaders.includePaths,
            project.qtbaseDir + "/mkspecs/" + Qt.global.privateConfig.hostMkspec)
        commonCppDefines: [
            "QT_BOOTSTRAPPED",
            "QT_NO_CAST_TO_ASCII",
            'QT_VERSION_STR="' + project.version + '"',
            "QT_VERSION_MAJOR=" + project.versionParts[0],
            "QT_VERSION_MINOR=" + project.versionParts[1],
            "QT_VERSION_PATCH=" + project.versionParts[2],
        ]
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII", "QT_NO_FOREACH")
        qt_zlib.useBundledZlib: !Qt.global.privateConfig.system_zlib
            || Qt.global.config.cross_compile
        qt_zlib.useQtCore: false

        Properties {
            condition: qbs.targetOS.contains("windows")
            cpp.dynamicLibraries: [
                "advapi32",
                "netapi32",
                "ole32",
                "shell32",
                "user32",
            ]
        }

        Group {
            name: "sources"
            prefix: project.prefix
            files: [
                "codecs/qlatincodec.cpp",
                "codecs/qtextcodec.cpp",
                "codecs/qutfcodec.cpp",
                "global/qglobal.cpp",
                "global/qrandom.cpp",
                "global/qlogging.cpp",
                "global/qmalloc.cpp",
                "global/qnumeric.cpp",
                "global/qoperatingsystemversion.cpp",
                "io/qabstractfileengine.cpp",
                "io/qbuffer.cpp",
                "serialization/qdatastream.cpp",
                "io/qdebug.cpp",
                "io/qdir.cpp",
                "io/qdiriterator.cpp",
                "io/qfile.cpp",
                "io/qfiledevice.cpp",
                "io/qfileinfo.cpp",
                "io/qfilesystemengine.cpp",
                "io/qfilesystementry.cpp",
                "io/qfsfileengine.cpp",
                "io/qfsfileengine_iterator.cpp",
                "io/qiodevice.cpp",
                "io/qloggingcategory.cpp",
                "io/qloggingregistry.cpp",
                "io/qstandardpaths.cpp",
                "io/qtemporaryfile.cpp",
                "serialization/qtextstream.cpp",
                "serialization/qjsonarray.cpp",
                "serialization/qjson.cpp",
                "serialization/qjsondocument.cpp",
                "serialization/qjsonobject.cpp",
                "serialization/qjsonparser.cpp",
                "serialization/qjsonvalue.cpp",
                "serialization/qjsonwriter.cpp",
                "kernel/qcoreapplication.cpp",
                "kernel/qcoreglobaldata.cpp",
                "kernel/qmetatype.cpp",
                "kernel/qsystemerror.cpp",
                "kernel/qvariant.cpp",
                "plugin/quuid.cpp",
                "tools/qarraydata.cpp",
                "tools/qbitarray.cpp",
                "tools/qbytearray.cpp",
                "tools/qbytearraymatcher.cpp",
                "tools/qcommandlineoption.cpp",
                "tools/qcommandlineparser.cpp",
                "tools/qcryptographichash.cpp",
                "tools/qdatetime.cpp",
                "tools/qhash.cpp",
                "tools/qline.cpp",
                "tools/qlinkedlist.cpp",
                "tools/qlist.cpp",
                "tools/qlocale.cpp",
                "tools/qlocale_tools.cpp",
                "tools/qmap.cpp",
                "tools/qpoint.cpp",
                "tools/qrect.cpp",
                "tools/qregexp.cpp",
                "tools/qringbuffer.cpp",
                "tools/qsize.cpp",
                "tools/qstring_compat.cpp",
                "tools/qstring.cpp",
                "tools/qstringbuilder.cpp",
                "tools/qstringlist.cpp",
                "tools/qvsnprintf.cpp",
                "tools/qversionnumber.cpp",
                "serialization/qxmlstream.cpp",
                "serialization/qxmlutils.cpp",
                "../xml/sax/qxml.cpp", // ### to be removed once linguist is migrated to qxmlstream
            ]
        }

        Group {
            name: "sources_windows"
            condition: qbs.targetOS.contains("windows")
            prefix: parent.prefix
            files: [
                "global/qoperatingsystemversion_win.cpp",
                "io/qfilesystemengine_win.cpp",
                "io/qfilesystemiterator_win.cpp",
                "io/qfsfileengine_win.cpp",
                "io/qstandardpaths_win.cpp",
                "kernel/qcoreapplication_win.cpp",
                "plugin/qsystemlibrary.cpp",
            ]
        }

        Group {
            name: "sources_darwin"
            condition: qbs.targetOS.contains("darwin")
            prefix: parent.prefix
            files: [
                "global/qoperatingsystemversion_darwin.mm",
                "kernel/qcoreapplication_mac.cpp",
                "kernel/qcore_foundation.mm",
                "kernel/qcore_mac.cpp",
                "kernel/qcore_mac_objc.mm",
            ]
        }

        Group {
            name: "sources_unix"
            condition: qbs.targetOS.contains("unix")
            prefix: parent.prefix
            files: [
                "io/qfilesystemengine_unix.cpp",
                "io/qfilesystemiterator_unix.cpp",
                "io/qfsfileengine_unix.cpp",
                "io/qstandardpaths_unix.cpp",
                "kernel/qcore_unix.cpp",
            ]
        }

        Export {
            Depends { name: "cpp" }
            Depends { name: "QtCoreHeaders" }
            Depends { name: "QtXmlHeaders" }
            prefixMapping: base.concat([{
                prefix: project.qtbaseShadowDir,
                replacement: qbs.installPrefix
            }])
            cpp.includePaths: project.includePaths.concat(
                QtCoreHeaders.includePaths,
                QtXmlHeaders.includePaths,
                project.qtbaseDir + "/mkspecs/" + Qt.global.privateConfig.hostMkspec)
            cpp.cxxLanguageVersion: "c++11"

            Depends { name: "Qt.global" }
            Depends { name: "qt_zlib" }
            qt_zlib.useBundledZlib: !Qt.global.privateConfig.system_zlib
                || Qt.global.config.cross_compile
            qt_zlib.useQtCore: false

            Properties {
                condition: qbs.targetOS.contains("darwin")
                cpp.frameworks: [
                    "CoreFoundation",
                    "Foundation"
                ].concat(qbs.targetOS.contains("macos") ? ["CoreServices"] : [])
            }
        }
    }
}

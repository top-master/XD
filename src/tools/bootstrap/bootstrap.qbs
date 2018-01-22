import qbs
import qbs.File
import qbs.FileInfo
import QtUtils

QtModuleProject {
    name: "QtBootstrap"
    simpleName: "bootstrap"
    prefix: project.qtbaseDir + "/src/corelib/"
    includePaths: QtUtils.includesForModule("QtCore-private", project.buildDirectory + '/' + simpleName, project.version)
    .concat(QtUtils.includesForModule("QtXml-private", project.buildDirectory + '/' + simpleName, project.version))
    .concat(project.qtbaseShadowDir + "/include")
    .concat(project.qtbaseShadowDir + "/include/QtCore")

    QtHeaders {
        name: project.headersName
        install: false
        sync.module: "QtCore"
        sync.prefix: project.simpleName
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

        files: []
        excludeFiles: []

        Group {
            name: "generated headers"
            prefix: project.qtbaseShadowDir + "/src/corelib/global/"
            files: [
                "qconfig.h",
            ]
        }

        Group {
            name: "headers"
            prefix: project.prefix
            files: [
                "arch/qatomic_bootstrap.h",
                "codecs/qtextcodec.h",
                "codecs/qtextcodec_p.h",
                "codecs/qutfcodec_p.h",
                "global/qcompilerdetection.h",
                "global/qendian.h",
                "global/qflags.h",
                "global/qglobal.h",
                "global/qglobal_p.h",
                "global/qglobalstatic.h",
                "global/qhooks_p.h",
                "global/qisenum.h",
                "global/qlibraryinfo.h",
                "global/qlogging.h",
                "global/qnamespace.h",
                "global/qnumeric.h",
                "global/qnumeric_p.h",
                "global/qprocessordetection.h",
                "global/qsysinfo.h",
                "global/qsystemdetection.h",
                "global/qoperatingsystemversion.h",
                "global/qtypeinfo.h",
                "global/qtypetraits.h",
                "global/qt_windows.h",
                "global/qversiontagging.h",
                "io/qabstractfileengine_p.h",
                "io/qbuffer.h",
                "io/qdatastream.h",
                "io/qdebug.h",
                "io/qdebug_p.h",
                "io/qdir.h",
                "io/qdiriterator.h",
                "io/qfiledevice.h",
                "io/qfiledevice_p.h",
                "io/qfile.h",
                "io/qfileinfo.h",
                "io/qfileinfo_p.h",
                "io/qfile_p.h",
                "io/qfilesystemengine_p.h",
                "io/qfilesystementry_p.h",
                "io/qfilesystemiterator_p.h",
                "io/qfilesystemmetadata_p.h",
                "io/qfsfileengine_iterator_p.h",
                "io/qfsfileengine_p.h",
                "io/qiodevice.h",
                "io/qiodevice_p.h",
                "io/qloggingcategory.h",
                "io/qloggingregistry_p.h",
                "io/qprocess.h",
                "io/qprocess_p.h",
                "io/qsettings.h",
                "io/qstandardpaths.h",
                "io/qtemporaryfile.h",
                "io/qtemporaryfile_p.h",
                "io/qtextstream.h",
                "io/qtextstream_p.h",
                "io/qurl.h",
                "json/qjsonarray.h",
                "json/qjsondocument.h",
                "json/qjsonobject.h",
                "json/qjsonvalue.h",
                "kernel/qcoreapplication.h",
                "kernel/qcoreapplication_p.h",
                "kernel/qcoreglobaldata_p.h",
                "kernel/qcore_mac_p.h",
                "kernel/qcore_unix_p.h",
                "kernel/qelapsedtimer.h",
                "kernel/qfunctions_p.h",
                "kernel/qmath.h",
                "kernel/qmetaobject.h",
                "kernel/qmetaobject_moc_p.h",
                "kernel/qmetaobject_p.h",
                "kernel/qmetatype.h",
                "kernel/qmetatype_p.h",
                "kernel/qmetatypeswitcher_p.h",
                "kernel/qobjectdefs.h",
                "kernel/qobjectdefs_impl.h",
                "kernel/qobject.h",
                "kernel/qpointer.h",
                "kernel/qsystemerror_p.h",
                "kernel/qtranslator.h",
                "kernel/qtranslator_p.h",
                "kernel/qvariant.h",
                "kernel/qvariant_p.h",
                "plugin/qfactoryloader_p.h",
                "plugin/qlibrary.h",
                "plugin/qlibrary_p.h",
                "plugin/qplugin.h",
                "plugin/qsystemlibrary_p.h",
                "plugin/quuid.h",
                "thread/qatomic.h",
                "thread/qbasicatomic.h",
                "thread/qgenericatomic.h",
                "thread/qmutex.h",
                "thread/qmutexpool_p.h",
                "thread/qorderedmutexlocker_p.h",
                "thread/qreadwritelock.h",
                "thread/qthreadstorage.h",
                "tools/qalgorithms.h",
                "tools/qarraydata.h",
                "tools/qbitarray.h",
                "tools/qbytearray.h",
                "tools/qbytearray_p.h",
                "tools/qbytearraylist.h",
                "tools/qchar.h",
                "tools/qcommandlineoption.h",
                "tools/qcommandlineparser.h",
                "tools/qcontainerfwd.h",
                "tools/qcontiguouscache.h",
                "tools/qcryptographichash.h",
                "tools/qdatetime.h",
                "tools/qdatetimeparser_p.h",
                "tools/qdatetime_p.h",
                "tools/qeasingcurve.h",
                "tools/qhash.h",
                "tools/qhashfunctions.h",
                "tools/qiterator.h",
                "tools/qline.h",
                "tools/qlist.h",
                "tools/qlinkedlist.h",
                "tools/qlocale.h",
                "tools/qlocale_p.h",
                "tools/qlocale_tools_p.h",
                "tools/qmap.h",
                "tools/qmargins.h",
                "tools/qpair.h",
                "tools/qpoint.h",
                "tools/qrect.h",
                "tools/qrefcount.h",
                "tools/qregexp.h",
                "tools/qregularexpression.h",
                "tools/qringbuffer_p.h",
                "tools/qscopedpointer.h",
                "tools/qset.h",
                "tools/qshareddata.h",
                "tools/qsharedpointer.h",
                "tools/qsharedpointer_impl.h",
                "tools/qsimd_p.h",
                "tools/qsize.h",
                "tools/qstack.h",
                "tools/qstringbuilder.h",
                "tools/qstring.h",
                "tools/qstringiterator_p.h",
                "tools/qstringlist.h",
                "tools/qstringmatcher.h",
                "tools/qtimezone.h",
                "tools/qtools_p.h",
                "tools/qvarlengtharray.h",
                "tools/qvector.h",
                "xml/qxmlstream.h",
                "xml/qxmlutils_p.h",
            ]
        }
    }

    QtHeaders {
        name: "QtXmlBootstrapHeaders"
        install: false
        sync.module: "QtXml"
        sync.prefix: "bootstrap"
        baseDir: "../../xml"
    }

    QtModule {
        name: project.privateName
        hostBuild: true
        simpleName: parent.simpleName + "_private"
        type: ["staticlibrary", "prl", "pri"]

        Properties {
            condition: qbs.targetOS.contains("darwin")
            bundle.isBundle: false
        }

/* TODO. Possibly move to more generic place
    QMAKE_CFLAGS += $$QMAKE_CFLAGS_SPLIT_SECTIONS
    QMAKE_CXXFLAGS += $$QMAKE_CXXFLAGS_SPLIT_SECTIONS
*/

        // TODO: Move to more generic place? Also needed by bootstrap-dbus and potentially others.
        Properties {
            condition: QtGlobalConfig.release_tools
            qbs.buildVariant: "release"
        }
        qbs.buildVariant: original

        // Ignore debug_and_release for host tools.
        multiplexByQbsProperties: base.filter(function(name) { return name !== "buildVariants"; })

        Depends { name: project.headersName }
        Depends { name: "QtXmlBootstrapHeaders" }
        Depends { name: "cpp" }
        Depends { name: "QtGlobalConfig" }
        Depends { name: "qt_zlib" }

        cpp.includePaths: [project.qtbaseDir + "/mkspecs/" + QtGlobalPrivateConfig.hostMkspec].concat(project.includePaths)
        commonCppDefines: [
            "QT_BOOTSTRAPPED",
            "QT_NO_CAST_TO_ASCII",
            'QT_VERSION_STR="' + project.version + '"',
            "QT_VERSION_MAJOR=" + project.versionParts[0],
            "QT_VERSION_MINOR=" + project.versionParts[1],
            "QT_VERSION_PATCH=" + project.versionParts[2],
        ]
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII", "QT_NO_FOREACH")
        qt_zlib.useBundledZlib: !QtGlobalPrivateConfig.system_zlib || QtGlobalConfig.cross_compile
        qt_zlib.useQtCore: false

        Properties {
            condition: qbs.targetOS.contains("windows")
            cpp.dynamicLibraries: [
                "user32",
                "ole32",
                "advapi32",
                "shell32",
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
                "global/qlogging.cpp",
                "global/qmalloc.cpp",
                "global/qnumeric.cpp",
                "global/qoperatingsystemversion.cpp",
                "io/qabstractfileengine.cpp",
                "io/qbuffer.cpp",
                "io/qdatastream.cpp",
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
                "io/qtextstream.cpp",
                "json/qjsonarray.cpp",
                "json/qjson.cpp",
                "json/qjsondocument.cpp",
                "json/qjsonobject.cpp",
                "json/qjsonparser.cpp",
                "json/qjsonvalue.cpp",
                "json/qjsonwriter.cpp",
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
                "xml/qxmlstream.cpp",
                "xml/qxmlutils.cpp",
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
            ]
        }

        Export {
            Depends { name: "cpp" }
            Depends { name: "QtGlobalPrivateConfig" }
            prefixMapping: base.concat([{
                prefix: project.qtbaseShadowDir,
                replacement: qbs.installPrefix
            }])
            cpp.includePaths: [project.qtbaseDir + "/mkspecs/" + QtGlobalPrivateConfig.hostMkspec].concat(project.includePaths)
            cpp.cxxLanguageVersion: "c++11"

            Depends { name: "QtGlobalPrivateConfig" }
            Depends { name: "qt_zlib" }
            qt_zlib.useBundledZlib: !QtGlobalPrivateConfig.system_zlib || QtGlobalConfig.cross_compile
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

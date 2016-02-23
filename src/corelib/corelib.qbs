import qbs
import qbs.TextFile
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtCore"
    simpleName: "core"
    prefix: project.qtbasePrefix + "src/corelib/"

    Product {
        name: root.privateName
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
        sync.module: root.name
        sync.classNames: ({
            "qglobal.h": ["QtGlobal"],
            "qendian.h": ["QtEndian"],
            "qconfig.h": ["QtConfig"],
            "qplugin.h": ["QtPlugin"],
            "qalgorithms.h": ["QtAlgorithms"],
            "qcontainerfwd.h": ["QtContainerFwd"],
            "qdebug.h": ["QtDebug"],
            "qevent.h": ["QtEvents"],
            "qnamespace.h": ["Qt"],
            "qnumeric.h": ["QtNumeric"],
            "qvariant.h": ["QVariantHash", "QVariantList", "QVariantMap"],
            "qbytearray.h": ["QByteArrayData"],
            "qbytearraylist.h": ["QByteArrayList"],
        })
        ModuleHeaders { fileTags: "header_sync" }
    }

    QtModule {
        condition: project.core
        name: root.moduleName
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "corelib.pro"

        Export {
            Depends { name: "cpp" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }

        Depends { name: "double-conversion" }
        Depends { name: "harfbuzz" }
        Depends { name: "pcre" }
        Depends { name: "zlib" }
        Depends { name: "glib"; condition: project.glib }
        Depends { name: "forkfd"; condition: qbs.targetOS.contains("unix") }

        cpp.defines: {
            var defines = ["QT_BUILD_CORE_LIB"];
            if (project.icu)
                defines.push("QT_USE_ICU");
            if (qbs.targetOS.contains("unix")) {
                if (project.config.contains("poll_poll"))
                    defines.push("QT_HAVE_POLL");
                if (project.config.contains("poll_ppoll"))
                    defines.push("QT_HAVE_PPOLL");
                if (project.config.contains("poll_pollts"))
                    defines.push("QT_HAVE_POLL", "QT_HAVE_POLLTS");
            }
            return defines.concat(base);
        }

        cpp.dynamicLibraries: {
            var dynamicLibraries = base;
            if (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("android")) {
                dynamicLibraries.push("pthread");
                dynamicLibraries.push("dl");
            }
            if (qbs.targetOS.contains("windows")) {
                dynamicLibraries.push("shell32");
                dynamicLibraries.push("user32");
                if (qbs.targetOS.contains("winrt")) {
                    dynamicLibraries.push("runtimeobject");
                } else {
                    dynamicLibraries.push("advapi32");
                    dynamicLibraries.push("ws2_32");
                    dynamicLibraries.push("mpr");
                    dynamicLibraries.push("uuid");
                    dynamicLibraries.push("ole32");
                }
            }
            if (project.icu) {
                dynamicLibraries.push("icui18n");
                dynamicLibraries.push("icuuc");
                dynamicLibraries.push("icudata");
            }
            return dynamicLibraries;
        }

        cpp.frameworks: {
            var frameworks = base;
            if (qbs.targetOS.contains("darwin")) {
                frameworks.push("CoreFoundation", "Foundation");
                if (qbs.targetOS.contains("osx")) {
                    frameworks.push("AppKit", "CoreServices");
                } else {
                    frameworks.push("MobileCoreServices");
                    if (qbs.targetOS.contains("ios"))
                        frameworks.push("UIKit");
                }
            }
            return frameworks;
        }

        cpp.includePaths: root.includePaths.concat([root.prefix]).concat(base)

        ModuleHeaders {
            excludeFiles: {
                var excludeFiles = [root.prefix + "doc/**", root.prefix + "kernel/qobjectdefs.h"];
                if (!qbs.targetOS.contains("blackberry")) {
                    excludeFiles.push(root.prefix + "kernel/qeventdispatcher_blackberry_p.h");
                    excludeFiles.push(root.prefix + "kernel/qppsobject_p.h");
                    excludeFiles.push(root.prefix + "tools/qlocale_blackberry.h");
                }
                if (!qbs.targetOS.contains("osx")) {
                    excludeFiles.push(root.prefix + "io/qfilesystemwatcher_fsevents_p.h");
                }
                if (!qbs.targetOS.contains("darwin")) {
                    excludeFiles.push(root.prefix + "kernel/qeventdispatcher_cf_p.h");
                }
                if (!qbs.targetOS.contains("linux") && !(qbs.targetOS.contains("qnx") && project.inotify)) {
                    excludeFiles.push(root.prefix + "io/qfilesystemwatcher_inotify_p.h");
                }
                if (!qbs.targetOS.contains("unix")) {
                    excludeFiles.push(root.prefix + "kernel/qeventdispatcher_unix_p.h");
                }
                if (!qbs.targetOS.contains("windows") || qbs.targetOS.contains("winrt")) {
                    excludeFiles.push(root.prefix + "io/qwindowspipereader_p.h");
                    excludeFiles.push(root.prefix + "io/qwindowspipewriter_p.h");
                    excludeFiles.push(root.prefix + "io/qfilesystemwatcher_win_p.h");
                    excludeFiles.push(root.prefix + "io/qwinoverlappedionotifier_p.h");
                    excludeFiles.push(root.prefix + "kernel/qeventdispatcher_win_p.h");
                }
                if (qbs.targetOS.contains("winrt")) {
                    excludeFiles.push(root.prefix + "io/qfilesystemwatcher*.h");
                }
                if (!qbs.targetOS.contains("winrt")) {
                    excludeFiles.push(root.prefix + "kernel/qeventdispatcher_winrt_p.h");
                }
                if (!project.glib) {
                    excludeFiles.push(root.prefix + "kernel/qeventdispatcher_glib_p.h");
                }
                if (!project.kqueue) {
                    excludeFiles.push(root.prefix + "io/qfilesystemwatcher_kqueue_p.h");
                }
                return excludeFiles;
            }
        }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "animation/qabstractanimation.cpp",
                "animation/qanimationgroup.cpp",
                "animation/qparallelanimationgroup.cpp",
                "animation/qpauseanimation.cpp",
                "animation/qpropertyanimation.cpp",
                "animation/qsequentialanimationgroup.cpp",
                "animation/qvariantanimation.cpp",
                "codecs/qbig5codec.cpp",
                "codecs/qeucjpcodec.cpp",
                "codecs/qeuckrcodec.cpp",
                "codecs/qgb18030codec.cpp",
                "codecs/qisciicodec.cpp",
                "codecs/qjiscodec.cpp",
                "codecs/qjpunicode.cpp",
                "codecs/qlatincodec.cpp",
                "codecs/qsimplecodec.cpp",
                "codecs/qsjiscodec.cpp",
                "codecs/qtextcodec.cpp",
                "codecs/qtsciicodec.cpp",
                "codecs/qutfcodec.cpp",
                "global/archdetect.cpp",
                "global/qglobal.cpp",
                "global/qglobalstatic.cpp",
                "global/qhooks.cpp",
                "global/qlibraryinfo.cpp",
                "global/qlogging.cpp",
                "global/qmalloc.cpp",
                "global/qnumeric.cpp",
                "global/qversiontagging.cpp",
                "io/qabstractfileengine.cpp",
                "io/qbuffer.cpp",
                "io/qdatastream.cpp",
                "io/qdataurl.cpp",
                "io/qdebug.cpp",
                "io/qdir.cpp",
                "io/qdiriterator.cpp",
                "io/qfile.cpp",
                "io/qfiledevice.cpp",
                "io/qfileinfo.cpp",
                "io/qfileselector.cpp",
                "io/qfilesystemengine.cpp",
                "io/qfilesystementry.cpp",
                "io/qfilesystemwatcher.cpp",
                "io/qfilesystemwatcher_polling.cpp",
                "io/qfsfileengine.cpp",
                "io/qfsfileengine_iterator.cpp",
                "io/qiodevice.cpp",
                "io/qipaddress.cpp",
                "io/qlockfile.cpp",
                "io/qloggingcategory.cpp",
                "io/qloggingregistry.cpp",
                "io/qnoncontiguousbytedevice.cpp",
                "io/qprocess.cpp",
                "io/qresource.cpp",
                "io/qresource_iterator.cpp",
                "io/qsavefile.cpp",
                "io/qsettings.cpp",
                "io/qstandardpaths.cpp",
                "io/qstorageinfo.cpp",
                "io/qtemporarydir.cpp",
                "io/qtemporaryfile.cpp",
                "io/qtextstream.cpp",
                "io/qtldurl.cpp",
                "io/qurl.cpp",
                "io/qurlidna.cpp",
                "io/qurlquery.cpp",
                "io/qurlrecode.cpp",
                "itemmodels/qabstractitemmodel.cpp",
                "itemmodels/qabstractproxymodel.cpp",
                "itemmodels/qidentityproxymodel.cpp",
                "itemmodels/qitemselectionmodel.cpp",
                "itemmodels/qsortfilterproxymodel.cpp",
                "itemmodels/qstringlistmodel.cpp",
                "json/qjsonarray.cpp",
                "json/qjson.cpp",
                "json/qjsondocument.cpp",
                "json/qjsonobject.cpp",
                "json/qjsonparser.cpp",
                "json/qjsonvalue.cpp",
                "json/qjsonwriter.cpp",
                "kernel/qabstracteventdispatcher.cpp",
                "kernel/qabstractnativeeventfilter.cpp",
                "kernel/qbasictimer.cpp",
                "kernel/qcoreapplication.cpp",
                "kernel/qcoreevent.cpp",
                "kernel/qcoreglobaldata.cpp",
                "kernel/qeventloop.cpp",
                "kernel/qmath.cpp",
                "kernel/qmetaobjectbuilder.cpp",
                "kernel/qmetaobject.cpp",
                "kernel/qmetatype.cpp",
                "kernel/qmimedata.cpp",
                "kernel/qobjectcleanuphandler.cpp",
                "kernel/qobject.cpp",
                "kernel/qpointer.cpp",
                "kernel/qsharedmemory.cpp",
                "kernel/qsignalmapper.cpp",
                "kernel/qsocketnotifier.cpp",
                "kernel/qsystemerror.cpp",
                "kernel/qsystemsemaphore.cpp",
                "kernel/qtimer.cpp",
                "kernel/qtranslator.cpp",
                "kernel/qvariant.cpp",
                "mimetypes/mimetypes.qrc",
                "mimetypes/qmimedatabase.cpp",
                "mimetypes/qmimeglobpattern.cpp",
                "mimetypes/qmimemagicrule.cpp",
                "mimetypes/qmimemagicrulematcher.cpp",
                "mimetypes/qmimeprovider.cpp",
                "mimetypes/qmimetype.cpp",
                "mimetypes/qmimetypeparser.cpp",
                "plugin/qelfparser_p.cpp",
                "plugin/qfactoryinterface.cpp",
                "plugin/qfactoryloader.cpp",
                "plugin/qlibrary.cpp",
                "plugin/qmachparser.cpp",
                "plugin/qpluginloader.cpp",
                "plugin/quuid.cpp",
                "statemachine/qabstractstate.cpp",
                "statemachine/qabstracttransition.cpp",
                "statemachine/qeventtransition.cpp",
                "statemachine/qfinalstate.cpp",
                "statemachine/qhistorystate.cpp",
                "statemachine/qsignaltransition.cpp",
                "statemachine/qstate.cpp",
                "statemachine/qstatemachine.cpp",
                "thread/qatomic.cpp",
                "thread/qexception.cpp",
                "thread/qfutureinterface.cpp",
                "thread/qfuturewatcher.cpp",
                "thread/qmutex.cpp",
                "thread/qmutexpool.cpp",
                "thread/qreadwritelock.cpp",
                "thread/qresultstore.cpp",
                "thread/qrunnable.cpp",
                "thread/qsemaphore.cpp",
                "thread/qthread.cpp",
                "thread/qthreadpool.cpp",
                "thread/qthreadstorage.cpp",
                "tools/qarraydata.cpp",
                "tools/qbitarray.cpp",
                "tools/qbytearray.cpp",
                "tools/qbytearraylist.cpp",
                "tools/qbytearraymatcher.cpp",
                "tools/qcollator.cpp",
                "tools/qcommandlineoption.cpp",
                "tools/qcommandlineparser.cpp",
                "tools/qcontiguouscache.cpp",
                "tools/qcryptographichash.cpp",
                "tools/qdatetime.cpp",
                "tools/qdatetimeparser.cpp",
                "tools/qeasingcurve.cpp",
                "tools/qelapsedtimer.cpp",
                "tools/qfreelist.cpp",
                "tools/qharfbuzz.cpp",
                "tools/qhash.cpp",
                "tools/qline.cpp",
                "tools/qlinkedlist.cpp",
                "tools/qlist.cpp",
                "tools/qlocale.cpp",
                "tools/qlocale_tools.cpp",
                "tools/qmap.cpp",
                "tools/qmargins.cpp",
                "tools/qmessageauthenticationcode.cpp",
                "tools/qpoint.cpp",
                "tools/qqueue.cpp",
                "tools/qrect.cpp",
                "tools/qrefcount.cpp",
                "tools/qregexp.cpp",
                "tools/qregularexpression.cpp",
                "tools/qringbuffer.cpp",
                "tools/qscopedpointer.cpp",
                "tools/qscopedvaluerollback.cpp",
                "tools/qshareddata.cpp",
                "tools/qsharedpointer.cpp",
                "tools/qsimd.cpp",
                "tools/qsize.cpp",
                "tools/qstack.cpp",
                "tools/qstringbuilder.cpp",
                "tools/qstring_compat.cpp",
                "tools/qstring.cpp",
                "tools/qstringlist.cpp",
                "tools/qtextboundaryfinder.cpp",
                "tools/qtimeline.cpp",
                "tools/qtimezone.cpp",
                "tools/qtimezoneprivate.cpp",
                "tools/qunicodetools.cpp",
                "tools/qvector.cpp",
                "tools/qversionnumber.cpp",
                "tools/qvsnprintf.cpp",
                "xml/qxmlstream.cpp",
                "xml/qxmlutils.cpp",
            ]
        }

        Group {
            name: "sources_android"
            condition: qbs.targetOS.contains("android")
            prefix: root.prefix
            files: [
                "io/qstandardpaths_android.cpp",
                "kernel/qjni.cpp",
                "kernel/qjnihelpers.cpp",
                "kernel/qjnionload.cpp",
                "kernel/qsharedmemory_android.cpp",
                "kernel/qsystemsemaphore_android.cpp",
                "tools/qtimezoneprivate_android.cpp",
            ]
        }

        Group {
            name: "sources_darwin"
            condition: qbs.targetOS.contains("darwin")
            prefix: root.prefix
            files: [
                "io/qsettings_mac.cpp",
                "io/qstorageinfo_mac.cpp",
                "io/qurl_mac.mm",
                "kernel/qcoreapplication_mac.cpp",
                "kernel/qcore_mac.cpp",
                "kernel/qcore_mac_objc.mm",
                "kernel/qcfsocketnotifier.cpp",
                "kernel/qeventdispatcher_cf.mm",
                "tools/qelapsedtimer_mac.cpp",
                "tools/qbytearray_mac.mm",
                "tools/qstring_mac.mm",
                "tools/qtimezoneprivate_mac.mm"
            ]
        }

        Group {
            name: "sources_osx"
            condition: qbs.targetOS.contains("osx")
            prefix: root.prefix
            files: [
                "io/qfilesystemwatcher_fsevents.mm",
                "tools/qcollator_macx.cpp"
            ]
        }

        Group {
            name: "sources_haiku"
            condition: qbs.targetOS.contains("haiku")
            prefix: root.prefix
            files: [
                "io/qstandardpaths_haiku.cpp",
            ]
        }

        Group {
            name: "sources_nacl"
            condition: qbs.targetOS.contains("nacl")
            prefix: root.prefix
            files: [
                "kernel/qfunctions_nacl.cpp",
            ]
        }

        Group {
            name: "sources_unix"
            condition: qbs.targetOS.contains("unix")
            prefix: root.prefix
            files: [
                "io/forkfd_qt.cpp",
                "io/qfilesystemengine_unix.cpp",
                "io/qfilesystemiterator_unix.cpp",
                "io/qfsfileengine_unix.cpp",
                "io/qlockfile_unix.cpp",
                "io/qprocess_unix.cpp",
                "kernel/qcore_unix.cpp",
                "kernel/qcrashhandler.cpp",
                "kernel/qeventdispatcher_unix.cpp",
                "kernel/qtimerinfo_unix.cpp",
                "plugin/qlibrary_unix.cpp",
                "thread/qthread_unix.cpp",
                "thread/qwaitcondition_unix.cpp",
                "tools/qlocale_unix.cpp",
            ]
        }

        Group {
            name: "sources_linux"
            condition: qbs.targetOS.contains("linux") || (qbs.targetOS.contains("qnx") && project.inotify)
            prefix: root.prefix
            files: [
                "io/qfilesystemwatcher_inotify.cpp"
            ]
        }

        Group {
            name: "sources_poll_select"
            condition: project.config.contains("poll_select") && qbs.targetOS.contains("unix")
            prefix: root.prefix
            files: [
                "kernel/qpoll.cpp"
            ]
        }

        Group {
            name: "sources_unix!android"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("android")
            prefix: root.prefix
            files: [
                "io/qstandardpaths_unix.cpp",
                "kernel/qsharedmemory_posix.cpp",
                "kernel/qsharedmemory_systemv.cpp",
                "kernel/qsharedmemory_unix.cpp",
                "kernel/qsystemsemaphore_posix.cpp",
                "kernel/qsystemsemaphore_systemv.cpp",
                "kernel/qsystemsemaphore_unix.cpp"
            ]
        }

        Group {
            name: "sources_unix!darwin"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
            prefix: root.prefix
            files: [
                "io/qstorageinfo_unix.cpp",
                "tools/qelapsedtimer_unix.cpp"
            ]
        }

        Group {
            name: "sources_genericunix"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("osx") && !project.icu
            prefix: root.prefix
            files: [
                "tools/qcollator_posix.cpp"
            ]
        }

        Group {
            name: "sources_unix!android!darwin"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("android") && !qbs.targetOS.contains("darwin")
            prefix: root.prefix
            files: [
                "tools/qtimezoneprivate_tz.cpp"
            ]
        }

        Group {
            name: "sources_wince"
            condition: qbs.targetOS.contains("wince")
            prefix: root.prefix
            files: [
                "io/qprocess_wince.cpp",
                "kernel/qfunctions_wince.cpp",
            ]
        }

        Group {
            name: "sources_windows"
            condition: qbs.targetOS.contains("windows")
            prefix: root.prefix
            files: [
                "codecs/qwindowscodec.cpp",
                "io/qfilesystemengine_win.cpp",
                "io/qfilesystemiterator_win.cpp",
                "io/qfsfileengine_win.cpp",
                "io/qlockfile_win.cpp",
                "kernel/qcoreapplication_win.cpp",
                "kernel/qsharedmemory_win.cpp",
                "kernel/qsystemsemaphore_win.cpp",
                "kernel/qwineventnotifier.cpp",
                "plugin/qlibrary_win.cpp",
                "plugin/qsystemlibrary.cpp",
                "thread/qwaitcondition_win.cpp",
                "thread/qthread_win.cpp",
                "tools/qlocale_win.cpp",
                "tools/qcollator_win.cpp",
                "tools/qelapsedtimer_win.cpp",
                "tools/qvector_msvc.cpp",
            ]
        }

        Group {
            name: "sources_winrt"
            condition: qbs.targetOS.contains("winrt")
            prefix: root.prefix
            files: [
                "io/qsettings_winrt.cpp",
                "io/qstandardpaths_winrt.cpp",
                "io/qstorageinfo_stub.cpp",
                "kernel/qeventdispatcher_winrt.cpp",
                "kernel/qfunctions_winrt.cpp",
            ]
        }

        Group {
            name: "sources_windows!winrt"
            condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")
            prefix: root.prefix
            files: [
                "io/qfilesystemwatcher_win.cpp",
                "io/qprocess_win.cpp",
                "io/qsettings_win.cpp",
                "io/qstandardpaths_win.cpp",
                "io/qstorageinfo_win.cpp",
                "io/qwinoverlappedionotifier.cpp",
                "io/qwindowspipereader.cpp",
                "io/qwindowspipewriter.cpp",
                "kernel/qeventdispatcher_win.cpp",
                "tools/qtimezoneprivate_win.cpp",
            ]
        }

        Group {
            name: "sources_vxworks"
            condition: qbs.targetOS.contains("vxworks")
            prefix: root.prefix
            files: [
                "kernel/qfunctions_vxworks.cpp",
            ]
        }

        Group {
            name: "sources_glib"
            condition: project.glib
            prefix: root.prefix
            files: [
                "kernel/qeventdispatcher_glib.cpp",
            ]
        }

        Group {
            name: "sources_iconv"
            condition: project.iconv
            prefix: root.prefix
            files: [
                "codecs/qiconvcodec.cpp",
            ]
        }

        Group {
            name: "sources_icu"
            condition: project.icu
            prefix: root.prefix
            cpp.defines: outer.concat(["QT_USE_ICU"])
            files: [
                "codecs/qicucodec.cpp",
                "tools/qcollator_icu.cpp",
                "tools/qlocale_icu.cpp",
                "tools/qtimezoneprivate_icu.cpp",
            ]
        }

        Group {
            name: "sources_kqueue"
            condition: project.kqueue
            prefix: root.prefix
            files: [
                "io/qfilesystemwatcher_kqueue.cpp"
            ]
        }

        Group {
            name: "mkspecs"
            // ### fixme files: project.qtbasePrefix + "mkspecs/**/*"
            qbs.install: true
            qbs.installDir: "mkspecs"
            qbs.installSourceBase: project.qtbasePrefix + "mkspecs"
        }
    }
}

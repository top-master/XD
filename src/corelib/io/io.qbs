import qbs

Group {
    name: "I/O"
    prefix: path + '/'
    cpp.includePaths: outer.concat(qbs.targetOS.contains("unix") && !qbs.targetOS.contains("integrity") ? ["../3rdparty/forkfd"] : [])
    files: [
        "qabstractfileengine.cpp",
        "qabstractfileengine_p.h",
        "qbuffer.cpp",
        "qbuffer.h",
        "qdataurl.cpp",
        "qdataurl_p.h",
        "qdebug.cpp",
        "qdebug.h",
        "qdebug_p.h",
        "qdir.cpp",
        "qdir.h",
        "qdir_p.h",
        "qdiriterator.cpp",
        "qdiriterator.h",
        "qfile.cpp",
        "qfile.h",
        "qfiledevice.cpp",
        "qfiledevice.h",
        "qfiledevice_p.h",
        "qfileinfo.cpp",
        "qfileinfo.h",
        "qfileinfo_p.h",
        "qfileselector.cpp",
        "qfileselector.h",
        "qfileselector_p.h",
        "qfilesystemengine.cpp",
        "qfilesystemengine_p.h",
        "qfilesystementry.cpp",
        "qfilesystementry_p.h",
        "qfilesystemiterator_p.h",
        "qfilesystemmetadata_p.h",
        "qfilesystemwatcher.cpp",
        "qfilesystemwatcher.h",
        "qfilesystemwatcher_p.h",
        "qfilesystemwatcher_polling.cpp",
        "qfilesystemwatcher_polling_p.h",
        "qfsfileengine.cpp",
        "qfsfileengine_p.h",
        "qfsfileengine_iterator.cpp",
        "qfsfileengine_iterator_p.h",
        "qiodevice.cpp",
        "qiodevice.h",
        "qiodevice_p.h",
        "qipaddress.cpp",
        "qipaddress_p.h",
        "qlockfile.cpp",
        "qlockfile.h",
        "qlockfile_p.h",
        "qloggingcategory.cpp",
        "qloggingcategory.h",
        "qloggingregistry_p.h",
        "qloggingregistry.cpp",
        "qnoncontiguousbytedevice.cpp",
        "qnoncontiguousbytedevice_p.h",
        "qresource.cpp",
        "qresource_p.h",
        "qresource_iterator.cpp",
        "qresource_iterator_p.h",
        "qsavefile.cpp",
        "qsavefile.h",
        "qsettings.cpp",
        "qsettings.h",
        "qsettings_p.h",
        "qstandardpaths.cpp",
        "qstandardpaths.h",
        "qstorageinfo.cpp",
        "qstorageinfo.h",
        "qstorageinfo_p.h",
        "qtemporarydir.cpp",
        "qtemporarydir.h",
        "qtemporaryfile.cpp",
        "qtemporaryfile.h",
        "qtemporaryfile_p.h",
        "qtldurl.cpp",
        "qtldurl_p.h",
        "qurl.cpp",
        "qurl.h",
        "qurl_p.h",
        "qurlidna.cpp",
        "qurlquery.cpp",
        "qurlquery.h",
        "qurlrecode.cpp",
        "qurltlds_p.h",
    ]
    Group {
        name: "I/O (unix)"
        condition: qbs.targetOS.contains("unix")
        files: [
            "qfilesystemengine_unix.cpp",
            "qfilesystemiterator_unix.cpp",
            "qfsfileengine_unix.cpp",
            "qlockfile_unix.cpp",
        ]
        Group {
            name: "I/O (unix, non-integrity, non-UIKit)"
            condition: !qbs.targetOS.contains("integrity") && !product.hasUiKit
            files: [
                "../../3rdparty/forkfd/forkfd.h",
                "forkfd_qt.cpp",
            ]
        }
        Group {
            name: "I/O (unix/non-darwin)"
            condition: !qbs.targetOS.contains("darwin")
            files: "qstorageinfo_unix.cpp"
        }
        Group {
            name: "I/O (darwin)"
            condition: qbs.targetOS.contains("darwin")
            files: [
                "qstandardpaths_mac.mm",
                "qstorageinfo_mac.cpp",
            ]
            Group {
                name: "I/O (macOS)"
                condition: qbs.targetOS.contains("macos")
                files: [
                    "qfilesystemwatcher_fsevents.mm",
                    "qfilesystemwatcher_fsevents_p.h",
                ]
            }
            Group {
                name: "I/O (darwin/non-nacl)"
                condition: true // TODO: !nacl
                files: "qsettings_mac.cpp"
            }
        }
        Group {
            name: "I/O (android)"
            condition: qbs.targetOS.contains("android") && !qbs.targetOS.contains("android-embedded")
            files: "qstandardpaths_android.cpp"
        }
        Group {
            name: "I/O (haiku)"
            condition: qbs.targetOS.contains("haiku")
            files: "qstandardpaths_haiku.cpp"
        }
        Group {
            name: "I/O (generic unix)"
            condition: !qbs.targetOS.contains("darwin")
                           && !qbs.targetOS.contains("android")
                           && !qbs.targetOS.contains("haiku")
            files: "qstandardpaths_unix.cpp"
        }
        Group {
            name: "I/O (bsd, non-nacl)"
            condition: qbs.targetOS.contains("bsd") || qbs.targetOS.contains("darwin") // TODO: !nacl
            files: [
                "qfilesystemwatcher_kqueue.cpp",
                "qfilesystemwatcher_kqueue_p.h",
            ]
        }
    }
    Group {
        name: "I/O (windows)"
        condition: qbs.targetOS.contains("windows")
        files: [
            "qfilesystemengine_win.cpp",
            "qfilesystemiterator_win.cpp",
            "qfilesystemwatcher_win.cpp",
            "qfilesystemwatcher_win_p.h",
            "qfsfileengine_win.cpp",
            "qlockfile_win.cpp",
        ]
        Group {
            name: "I/O (win32)"
            condition: !product.targetsUWP
            files: [
                "qsettings_win.cpp",
                "qstandardpaths_win.cpp",
                "qstorageinfo_win.cpp",
                "qwindowspipereader.cpp",
                "qwindowspipereader_p.h",
                "qwindowspipewriter.cpp",
                "qwindowspipewriter_p.h",
            ]
        }
        Group {
            name: "I/O (winrt)"
            condition: product.targetsUWP
            files: [
                "qsettings_winrt.cpp",
                "qstandardpaths_winrt.cpp",
                "qstorageinfo_stub.cpp",
            ]
        }
    }
    Group {
        name: "qprocess"
        condition: product.config.processenvironment
        files: [
            "qprocess.cpp",
            "qprocess.h",
            "qprocess_p.h",
        ]
        Group {
            name: "qprocess/windows"
            condition: qbs.targetOS.contains("windows") && !product.targetsUWP
            files: "qprocess_win.cpp"
        }
        Group {
            name: "qprocess/unix"
            condition: qbs.targetOS.contains("unix")
            files: "qprocess_unix.cpp"
            Group {
                name: "qprocess/darwin"
                condition: qbs.targetOS.contains("darwin")
                files: "qprocess_darwin.mm"
            }
        }
    }
    Group {
        name: "inotify"
        condition: qbs.targetOS.contains("linux")
                       || (qbs.targetOS.contains("qnx") && QtCorePrivateConfig.inotify)
        files: [
            "qfilesystemwatcher_inotify.cpp",
            "qfilesystemwatcher_inotify_p.h",
        ]
    }
}

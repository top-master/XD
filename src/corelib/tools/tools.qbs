import qbs

Group {
    name: "tool classes"
    prefix: path + '/'
    files: [
        "qalgorithms.h",
        "qarraydataops.h",
        "qarraydatapointer.h",
        "qarraydata.cpp",
        "qarraydata.h",
        "qbitarray.cpp",
        "qbitarray.h",
        "qbytearray.cpp",
        "qbytearray.h",
        "qbytearray_p.h",
        "qbytearraylist.cpp",
        "qbytearraylist.h",
        "qbytearraymatcher.cpp",
        "qbytearraymatcher.h",
        "qbytedata_p.h",
        "qcache.h",
        "qchar.h",
        "qcollator.cpp",
        "qcollator.h",
        "qcollator_p.h",
        "qcontainerfwd.h",
        "qcontiguouscache.cpp",
        "qcontiguouscache.h",
        "qcryptographichash.cpp",
        "qcryptographichash.h",
        "qdatetime.cpp",
        "qdatetime.h",
        "qdatetime_p.h",
        "qdatetimeparser.cpp",
        "qdatetimeparser_p.h",
        "qdoublescanprint_p.h",
        "qeasingcurve.cpp",
        "qeasingcurve.h",
        "qfreelist.cpp",
        "qfreelist_p.h",
        "qharfbuzz.cpp",
        "qhashfunctions.h",
        "qhash.cpp",
        "qhash.h",
        "qiterator.h",
        "qline.cpp",
        "qline.h",
        "qlinkedlist.cpp",
        "qlinkedlist.h",
        "qlist.cpp",
        "qlist.h",
        "qlocale.cpp",
        "qlocale.h",
        "qlocale_data_p.h",
        "qlocale_p.h",
        "qlocale_tools.cpp",
        "qlocale_tools_p.h",
        "qmap.cpp",
        "qmap.h",
        "qmargins.cpp",
        "qmargins.h",
        "qmessageauthenticationcode.cpp",
        "qmessageauthenticationcode.h",
        "qpair.h",
        "qpodlist_p.h",
        "qpoint.cpp",
        "qpoint.h",
        "qqueue.cpp",
        "qqueue.h",
        "qrect.cpp",
        "qrect.h",
        "qrefcount.cpp",
        "qrefcount.h",
        "qregexp.cpp",
        "qregexp.h",
        "qringbuffer.cpp",
        "qringbuffer_p.h",
        "qscopedpointer.cpp",
        "qscopedpointer.h",
        "qscopedpointer_p.h",
        "qscopedvaluerollback.cpp",
        "qscopedvaluerollback.h",
        "qset.h",
        "qshareddata.cpp",
        "qshareddata.h",
        "qsharedpointer.cpp",
        "qsharedpointer.h",
        "qsharedpointer_impl.h",
        "qsimd.cpp",
        "qsimd_p.h",
        "qsize.cpp",
        "qsize.h",
        "qstack.cpp",
        "qstack.h",
        "qstringalgorithms_p.h",
        "qstringbuilder.cpp",
        "qstringbuilder.h",
        "qstringiterator_p.h",
        "qstringmatcher.h",
        "qstring.cpp",
        "qstring.h",
        "qstringlist.cpp",
        "qstringlist.h",
        "qtextboundaryfinder.cpp",
        "qtextboundaryfinder.h",
        "qtimeline.cpp",
        "qtimeline.h",
        "qtools_p.h",
        "qunicodetables_p.h",
        "qunicodetools.cpp",
        "qunicodetools_p.h",
        "qvarlengtharray.h",
        "qvector.h",
        "qversionnumber.cpp",
        "qversionnumber.h",
        "qvsnprintf.cpp",
    ]

    Group {
        name: "tool classes/mac/non-nacl"
        condition: qbs.targetOS.contains("darwin") && true // TODO "!nacl"
        files: "qlocale_mac.mm"
    }
    Group {
        name: "tool classes/unix"
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
        files: ["qlocale_unix.cpp"]
    }
    Group {
        name: "tool classes/windows"
        condition: qbs.targetOS.contains("windows")
        files: ["qlocale_win.cpp"]
    }
    Group {
        name: "tool classes/icu"
        condition: product.privateConfig.icu
        files: [
            "qcollator_icu.cpp",
            "qlocale_icu.cpp",
        ]
    }
    Group {
        name: "collator/non-icu/windows"
        condition: qbs.targetOS.contains("windows") && !product.privateConfig.icu
        files: ["qcollator_win.cpp"]
    }
    Group {
        name: "collator/non-icu/macos"
        condition: qbs.targetOS.contains("macos") && !product.privateConfig.icu
        files: ["qcollator_macx.cpp"]
    }
    Group {
        name: "collator/non-icu/unix"
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("macos")
            && !product.privateConfig.icu
        files: ["qcollator_posix.cpp"]
    }

    Group {
        name: "regularexpression"
        condition: product.config.regularexpression
        files: [
            "qregularexpression.cpp",
            "qregularexpression.h",
        ]
    }

    Group {
        name: "commandlineparser"
        condition: product.config.commandlineparser
        files: [
            "qcommandlineoption.cpp",
            "qcommandlineoption.h",
            "qcommandlineparser.cpp",
            "qcommandlineparser.h",
        ]
    }

    Group {
        name: "timezone"
        condition: product.config.timezone
        files: [
            "qtimezoneprivate.cpp",
            "qtimezoneprivate_p.h",
            "qtimezoneprivate_data_p.h",
            "qtimezone.cpp",
            "qtimezone.h",
        ]
        Group {
            name: "timezone/darwin"
            condition: qbs.targetOS.contains("darwin") // and: "!nacl"
            files: ["qtimezoneprivate_mac.mm"]
        }
        Group {
            name: "timezone/android"
            condition: qbs.targetOS.contains("android")
            files: ["qtimezoneprivate_android.cpp"]
        }
        Group {
            name: "timezone/unix"
            condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
                           && !qbs.targetOS.contains("android")
            files: ["qtimezoneprivate_tz.cpp"]
        }
        Group {
            name: "timezone/windows"
            condition: qbs.targetOS.contains("windows")
            files: ["qtimezoneprivate_win.cpp"]
        }
        Group {
            name: "timezone/icu"
            condition: product.privateConfig.icu
            files: ["qtimezoneprivate_icu.cpp"]
        }
    }

    Group {
        name: "sources_no_pch"
        cpp.useCxxPrecompiledHeader: false
        files: ["qstring_compat.cpp"]
        Group {
            name: "sources_no_pch_windows"
            condition: qbs.toolchain.contains("msvc")
            files: ["qvector_msvc.cpp"]
        }
    }

    // TODO: SIMD-specific MIPS sources
}

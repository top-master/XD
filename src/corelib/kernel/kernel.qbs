import qbs

Group {
    name: "kernel"
    prefix: path + '/'
    files: [
        "qabstracteventdispatcher.cpp",
        "qabstracteventdispatcher.h",
        "qabstracteventdispatcher_p.h",
        "qabstractnativeeventfilter.cpp",
        "qabstractnativeeventfilter.h",
        "qbasictimer.cpp",
        "qbasictimer.h",
        "qcoreapplication.cpp",
        "qcoreapplication.h",
        "qcoreapplication_p.h",
        "qcorecmdlineargs_p.h",
        "qcoreevent.cpp",
        "qcoreevent.h",
        "qcoreglobaldata.cpp",
        "qcoreglobaldata_p.h",
        "qdeadlinetimer.cpp",
        "qdeadlinetimer.h",
        "qdeadlinetimer_p.h",
        "qelapsedtimer.cpp",
        "qelapsedtimer.h",
        "qeventloop.cpp",
        "qeventloop.h",
        "qfunctions_p.h",
        "qmath.cpp",
        "qmath.h",
        "qmetaobject.cpp",
        "qmetaobject.h",
        "qmetaobject_p.h",
        "qmetaobjectbuilder.cpp",
        "qmetaobjectbuilder_p.h",
        "qmetaobject_moc_p.h",
        "qmetatype.cpp",
        "qmetatypeswitcher_p.h",
        "qmetatype.h",
        "qmetatype_p.h",
        "qmimedata.cpp",
        "qmimedata.h",
        "qobjectdefs_impl.h",
        "qobject.cpp",
        "qobject.h",
        "qobject_impl.h",
        "qobject_p.h",
        "qobjectcleanuphandler.cpp",
        "qobjectcleanuphandler.h",
        "qpointer.cpp",
        "qpointer.h",
        "qsharedmemory.cpp",
        "qsharedmemory.h",
        "qsharedmemory_p.h",
        "qsignalmapper.cpp",
        "qsignalmapper.h",
        "qsocketnotifier.cpp",
        "qsocketnotifier.h",
        "qsystemerror.cpp",
        "qsystemerror_p.h",
        "qsystemsemaphore.cpp",
        "qsystemsemaphore.h",
        "qsystemsemaphore_p.h",
        "qtimer.cpp",
        "qtimer.h",
        "qtranslator.cpp",
        "qtranslator.h",
        "qtranslator_p.h",
        "qvariant.cpp",
        "qvariant.h",
        "qvariant_p.h",
    ]
    Group {
        name: "kernel (unmocable headers)"
        files: [
            "qobjectdefs.h",
        ]
        fileTags: ["unmocable"]
        overrideTags: false
    }
    Group {
        name: "kernel/windows"
        condition: qbs.targetOS.contains("windows")
        files: [
            "qcoreapplication_win.cpp",
            "qelapsedtimer_win.cpp",
            "qsharedmemory_win.cpp",
            "qsystemsemaphore_win.cpp",
            "qwineventnotifier.cpp",
            "qwineventnotifier.h",
        ]
        Group {
            name: parent.name + " (winrt)"
            condition: product.targetsUWP
            files: [
                "qeventdispatcher_winrt.cpp",
                "qeventdispatcher_winrt_p.h",
                "qfunctions_fake_env_p.h",
                "qfunctions_winrt.cpp",
                "qfunctions_winrt.h",
            ]
        }
        Group {
            name: parent.name + " (win32)"
            condition: !product.targetsUWP
            files: [
                "qeventdispatcher_win.cpp",
                "qeventdispatcher_win_p.h",
            ]
        }
    }
    Group {
        name: "kernel/unix"
        condition: qbs.targetOS.contains("unix")
        files: [
            "qcore_unix.cpp",
            "qcore_unix_p.h",
            "qeventdispatcher_unix.cpp",
            "qeventdispatcher_unix_p.h",
            "qpoll_p.h",
            "qtimerinfo_unix.cpp",
            "qtimerinfo_unix_p.h",
        ]
        Group {
            name: "kernel/darwin"
            condition: qbs.targetOS.contains("darwin")
            files: [
                "qcfsocketnotifier.cpp",
                "qcfsocketnotifier_p.h",
                "qcoreapplication_mac.cpp",
                "qcore_foundation.mm",
                "qcore_mac.cpp",
                "qcore_mac_p.h",
                "qcore_mac_objc.mm",
                "qeventdispatcher_cf.mm",
                "qeventdispatcher_cf_p.h",
            ]
            Group {
                name: "kernel/darwin/non-nacl"
                condition: true // TODO: !nacl
                files: "qelapsedtimer_mac.cpp"
            }
        }
        Group {
            name: "kernel/non-darwin"
            condition: !qbs.targetOS.contains("darwin") // TODO: nacl
            files: "qelapsedtimer_unix.cpp"
        }
        Group {
            name: "kernel/android"
            condition: qbs.targetOS.contains("android")
            files: [
                "qjnihelpers.cpp",
                "qjnionload.cpp",
                "qjni.cpp",
                "qsharedmemory_android.cpp",
                "qsystemsemaphore_android.cpp",
            ]
        }
        Group {
            name: "kernel/unix/non-android"
            condition: !qbs.targetOS.contains("android")
            files: [
                "qsharedmemory_posix.cpp",
                "qsharedmemory_systemv.cpp",
                "qsharedmemory_unix.cpp",
                "qsystemsemaphore_posix.cpp",
                "qsystemsemaphore_systemv.cpp",
                "qsystemsemaphore_unix.cpp",
            ]
        }
        Group {
            name: "kernel/vxworks"
            condition: qbs.targetOS.contains("vxworks")
            files: "qfunctions_vxworks.cpp"
        }
        Group {
            name: "kernel/qnx_pps"
            condition: qbs.targetOS.contains("qnx") && QtCorePrivateConfig.qqnx_pps
            files: [
                "qppsattribute.cpp",
                "qppsattribute_p.h",
                "qppsattributeprivate_p.h",
                "qppsobjectprivate_p.h",
                "qppsobject.cpp",
                "qppsobject_p.h",
            ]
        }
    }
    Group {
        name: "kernel/nacl"
        condition: false // TODO: nacl
        files: [
            "qfunctions_nacl.cpp",
            "qfunctions_nacl.h",
        ]
    }
    Group {
        name: "kernel/generic elapsed timer"
        condition: !qbs.targetOS.contains("windows") && !qbs.targetOS.contains("unix")
        files: "qelapsedtimer_generic.cpp"
    }
    Group {
        name: "kernel/poll_select"
        condition: QtCorePrivateConfig.poll_select
        files: "qpoll.cpp"
    }
    Group {
        name: "kernel/glib"
        condition: QtCorePrivateConfig.glib
        files: [
            "qeventdispatcher_glib.cpp",
            "qeventdispatcher_glib_p.h",
        ]
    }
}

import qbs

Group {
    name: "thread"
    prefix: path + '/'
    files: [
        "qatomic.cpp",
        "qatomic.h",
        "qatomic_msvc.h",
        "qbasicatomic.h",
        "qgenericatomic.h",
        "qmutex.cpp",
        "qmutex.h",
        "qmutex_p.h",
        "qmutexpool.cpp",
        "qmutexpool_p.h",
        "qorderedmutexlocker_p.h",
        "qreadwritelock.cpp",
        "qreadwritelock.h",
        "qreadwritelock_p.h",
        "qrunnable.cpp",
        "qrunnable.h",
        "qsemaphore.cpp",
        "qsemaphore.h",
        "qthread.cpp",
        "qthread.h",
        "qthread_p.h",
        "qthreadpool.cpp",
        "qthreadpool.h",
        "qthreadpool_p.h",
        "qthreadstorage.cpp",
        "qthreadstorage.h",
        "qwaitcondition.h",
    ]
    Group {
        condition: Qt.global.config.future
        name: "future"
        files: [
            "qexception.cpp",
            "qexception.h",
            "qfuture.h",
            "qfutureinterface.cpp",
            "qfutureinterface.h",
            "qfutureinterface_p.h",
            "qfuturesynchronizer.h",
            "qfuturewatcher.cpp",
            "qfuturewatcher.h",
            "qfuturewatcher_p.h",
            "qresultstore.cpp",
            "qresultstore.h",
        ]
    }
    Group {
        name: "thread/windows"
        condition: qbs.targetOS.contains("windows")
        files: [
            "qthread_win.cpp",
            "qwaitcondition_win.cpp",
        ]
    }
    Group {
        name: "qmutex/windows"
        condition: qbs.targetOS.contains("windows")
        files: "qmutex_win.cpp"
        fileTags: "hpp"
    }
    Group {
        name: "thread/unix"
        condition: qbs.targetOS.contains("unix")
        files: [
            "qthread_unix.cpp",
            "qwaitcondition_unix.cpp",
        ]
        Group {
            name: "qmutex_linux"
            condition: qbs.targetOS.contains("linux")
            files: "qmutex_linux.cpp"
            fileTags: "hpp"
        }
        Group {
            name: "thread/darwin"
            condition: qbs.targetOS.contains("darwin")
            files: "qmutex_mac.cpp"
            fileTags: "hpp"
        }
        Group {
            name: "thread/unix/generic"
            condition: !qbs.targetOS.contains("linux") && !qbs.targetOS.contains("darwin")
            files: "qmutex_unix.cpp"
            fileTags: "hpp"
        }
    }
}

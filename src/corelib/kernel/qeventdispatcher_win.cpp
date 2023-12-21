/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeventdispatcher_win_p.h"

#include "qcoreapplication.h"
#include <private/qsystemlibrary_p.h>
#include "qpair.h"
#include "qset.h"
#include "qsocketnotifier.h"
#include "qvarlengtharray.h"
#include "qwineventnotifier.h"

#include "qelapsedtimer.h"
#include "qcoreapplication_p.h"
#include <private/qthread_p.h>
#include <private/qmutexpool_p.h>

#include <WinSock2.h>
#include <WinUser.h>


QT_BEGIN_NAMESPACE

HINSTANCE qWinAppInst();
extern uint qGlobalPostedEventsCount();

#ifndef TIME_KILL_SYNCHRONOUS
#  define TIME_KILL_SYNCHRONOUS 0x0100
#endif

#ifndef QS_RAWINPUT
# ifdef Q_OS_WINCE
#  define QS_RAWINPUT 0x0000
# else
#  define QS_RAWINPUT 0x0400
# endif
#endif

#ifndef WM_TOUCH
#  define WM_TOUCH 0x0240
#endif
#ifndef QT_NO_GESTURES
#ifndef WM_GESTURE
#  define WM_GESTURE 0x0119
#endif
#ifndef WM_GESTURENOTIFY
#  define WM_GESTURENOTIFY 0x011A
#endif
#endif // QT_NO_GESTURES

enum {
    WM_QT_SOCKETNOTIFIER = WM_USER,
    WM_QT_SENDPOSTEDEVENTS = WM_USER + 1,
    WM_QT_ACTIVATENOTIFIERS = WM_USER + 2,
    SendPostedEventsWindowsTimerId = ~1u
};

#if defined(Q_OS_WINCE)
QT_BEGIN_INCLUDE_NAMESPACE
#include <winsock.h>
QT_END_INCLUDE_NAMESPACE
// Asynchronous Winsocks ------------------------------------------
#ifndef QT_NO_THREAD
QT_BEGIN_INCLUDE_NAMESPACE
#include <qthread.h>
#include <qmap.h>
#include <qmutex.h>
QT_END_INCLUDE_NAMESPACE

//#define QCE_ASYNC_DEBUG

namespace {
    class SocketAsyncHandler;

    class SocketAsyncHandler : public QThread
    {
    public:
        SocketAsyncHandler();
        ~SocketAsyncHandler();
        void run();
        void select(SOCKET sock, HWND handle, unsigned int msg, long ev);
        void removeSelect(SOCKET sock);
        void safeRemove(SOCKET sock);
    private:
        struct SockInfo {
            HWND handle;
            unsigned int msg;
            long ev;
        };
        QMap<SOCKET, SockInfo> sockets;
        QMutex mutex;
        QWaitCondition cond;
        bool supposedToDie;
    };

    SocketAsyncHandler::SocketAsyncHandler()
        : supposedToDie(false)
    {
    }

    SocketAsyncHandler::~SocketAsyncHandler()
    {
        mutex.lock();
        supposedToDie = true;
        mutex.unlock();
        cond.wakeOne();
        wait();
        while (sockets.size() > 0)
            removeSelect(sockets.begin().key());
    }

    void SocketAsyncHandler::removeSelect(SOCKET sock)
    {
        if (!sockets.contains(sock))
            return;
        sockets.remove(sock);
        return;
    }

    void SocketAsyncHandler::safeRemove(SOCKET sock)
    {
        QMutexLocker locker(&mutex);
        removeSelect(sock);
    }

    void SocketAsyncHandler::select(SOCKET sock, HWND handle, unsigned int msg, long ev)
    {
        QMutexLocker locker(&mutex);

        if (sockets.contains(sock))
            sockets.remove(sock);

        SockInfo info;
        info.handle = handle;
        info.msg = msg;
        info.ev = ev;
        sockets.insert(sock, info);
        cond.wakeOne();
    }

    void SocketAsyncHandler::run()
    {
        do {
            mutex.lock();

            while (!supposedToDie && sockets.isEmpty()) {
                cond.wait(&mutex);
            }

            if (supposedToDie) {
                mutex.unlock();
                break;
            }

            // Copy current items to reduce lock time
            // and to be able to use SendMessage
            QMap<SOCKET, SockInfo> currentSockets = sockets;
            mutex.unlock();

            fd_set readS, writeS, exS;
            FD_ZERO(&readS);
            FD_ZERO(&writeS);
            FD_ZERO(&exS);

            int maxFd = 0;

            for (QMap<SOCKET, SockInfo>::iterator it = currentSockets.begin(); it != currentSockets.end(); ++it) {
                const SockInfo &info = it.value();
                int socket = it.key();
                maxFd = qMax(maxFd, socket);

                if ((info.ev & FD_READ) || (info.ev & FD_CLOSE) || (info.ev & FD_ACCEPT))
                    FD_SET(socket, &readS);
                if ((info.ev & FD_WRITE)|| (info.ev & FD_CONNECT))
                    FD_SET(socket, &writeS);
                if (info.ev & FD_OOB)
                    FD_SET(socket, &exS);
            }

            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 50000;
            int result = ::select(maxFd + 1, &readS, &writeS, &exS, &timeout);
            if (result > 0) {
                HWND handle;
                unsigned int tmpMsg;
                SOCKET sock;
                HRESULT ret;
                for (QMap<SOCKET, SockInfo>::const_iterator it = currentSockets.constBegin();
                    it != currentSockets.constEnd(); ++it) {
                        handle = (*it).handle;
                        tmpMsg = (*it).msg;
                        sock = it.key();
                        if (FD_ISSET(sock, &readS))
                            ret = SendMessage(handle, tmpMsg, sock, FD_READ);

                        if (FD_ISSET(sock, &writeS))
                            ret = SendMessage(handle, tmpMsg, sock, FD_WRITE);

                        if (FD_ISSET(sock, &exS))
                            ret = SendMessage(handle, tmpMsg, sock, FD_OOB);
                }
            }

#ifdef QCE_ASYNC_DEBUG
            else if (result == 0) { //timeout
                qDebug("    WSAAsync select timeout");
            } else if (result < 0) { // SocketError
                // This might happen because of two reasons
                // 1. We already closed a socket in between the copy and the select
                //    and thus select() returns an error
                // 2. Something is really wrong, then
                //    ### Loop on all descriptors, try to select and remove the
                //    ### broken one.
                qWarning("WSAAsync select error %d", WSAGetLastError());
            }
#endif
        } while(true);
    }
} // namespace

Q_GLOBAL_STATIC(SocketAsyncHandler, qt_async_handler)

int WSAAsyncSelect(SOCKET sock, HWND handle, unsigned int msg, long ev)
{
    if (sock == 0 || handle == 0 || handle == INVALID_HANDLE_VALUE) {
        WSASetLastError(WSAEINVAL);
        return SOCKET_ERROR;
    }

    if (msg == 0 && ev == 0)
        qt_async_handler()->safeRemove(sock);
    else
        qt_async_handler()->select(sock, handle, msg, ev);

    qt_async_handler()->start(QThread::LowPriority);
    WSASetLastError(0);
    return 0;
}
#else // QT_NO_THREAD
int WSAAsyncSelect(SOCKET, HWND, unsigned int, long)
{
    return SOCKET_ERROR;
}
#endif
#endif // Q_OS_WINCE

class QEventDispatcherWin32Private;

#if !defined(DWORD_PTR) && !defined(Q_OS_WIN64)
#define DWORD_PTR DWORD
#endif

typedef MMRESULT(WINAPI *ptimeSetEvent)(UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT);
typedef MMRESULT(WINAPI *ptimeKillEvent)(UINT);

static ptimeSetEvent qtimeSetEvent = 0;
static ptimeKillEvent qtimeKillEvent = 0;

LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);

static void resolveTimerAPI()
{
    static bool triedResolve = false;
    if (!triedResolve) {
#ifndef QT_NO_THREAD
        QMutexLocker locker(QMutexPool::globalInstanceGet(&triedResolve));
        if (triedResolve)
            return;
#endif
        triedResolve = true;
#if !defined(Q_OS_WINCE)
        qtimeSetEvent = (ptimeSetEvent)QSystemLibrary::resolve(QLatin1String("winmm"), "timeSetEvent");
        qtimeKillEvent = (ptimeKillEvent)QSystemLibrary::resolve(QLatin1String("winmm"), "timeKillEvent");
#else
        qtimeSetEvent = (ptimeSetEvent)QSystemLibrary::resolve(QLatin1String("Mmtimer"), "timeSetEvent");
        qtimeKillEvent = (ptimeKillEvent)QSystemLibrary::resolve(QLatin1String("Mmtimer"), "timeKillEvent");
#endif
    }
}

QEventDispatcherWin32Private::QEventDispatcherWin32Private()
    : threadId(GetCurrentThreadId()), interrupt(false), closingDown(false), internalHwnd(0),
      getMessageHook(0), serialNumber(0), lastSerialNumber(0), sendPostedEventsWindowsTimerId(0),
      wakeUps(0)
#ifndef Q_OS_WINCE
    , activateNotifiersPosted(false)
#endif
{
    resolveTimerAPI();
}

QEventDispatcherWin32Private::~QEventDispatcherWin32Private()
{
    if (internalHwnd)
        DestroyWindow(internalHwnd);
}

void QEventDispatcherWin32Private::activateEventNotifier(QWinEventNotifier * wen)
{
    QEvent event(QEvent::WinEventAct);
    QCoreApplication::sendEvent(wen, &event);
}

// This function is called by a workerthread
void WINAPI QT_WIN_CALLBACK qt_fast_timer_proc(uint timerId, uint /*reserved*/, DWORD_PTR user, DWORD_PTR /*reserved*/, DWORD_PTR /*reserved*/)
{
    if (!timerId) // sanity check
        return;
    WinTimerInfo *t = (WinTimerInfo*)user;
    Q_ASSERT(t);
    QCoreApplication::postEvent(t->dispatcher, new QTimerEvent(t->timerId));
}

LRESULT QT_WIN_CALLBACK qt_internal_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    if (message == WM_NCCREATE)
        return true;

    MSG msg;
    msg.hwnd = hwnd;
    msg.message = message;
    msg.wParam = wp;
    msg.lParam = lp;
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
    long result;
    if (!dispatcher) {
        if (message == WM_TIMER)
            KillTimer(hwnd, wp);
        return 0;
    } else if (dispatcher->filterNativeEvent(QByteArrayLiteral("windows_dispatcher_MSG"), &msg, &result)) {
        return result;
    }

#ifdef GWLP_USERDATA
    QEventDispatcherWin32 *q = (QEventDispatcherWin32 *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
    QEventDispatcherWin32 *q = (QEventDispatcherWin32 *) GetWindowLong(hwnd, GWL_USERDATA);
#endif
    QEventDispatcherWin32Private *d = 0;
    if (q != 0)
        d = q->d_func();

    if (message == WM_QT_SOCKETNOTIFIER) {
        // socket notifier message
        int type = -1;
        switch (WSAGETSELECTEVENT(lp)) {
        case FD_READ:
        case FD_ACCEPT:
            type = 0;
            break;
        case FD_WRITE:
        case FD_CONNECT:
            type = 1;
            break;
        case FD_OOB:
            type = 2;
            break;
        case FD_CLOSE:
            type = 3;
            break;
        }
        if (type >= 0) {
            Q_ASSERT(d != 0);
            QSNDict *sn_vec[4] = { &d->sn_read, &d->sn_write, &d->sn_except, &d->sn_read };
            QSNDict *dict = sn_vec[type];

            QSockNot *sn = dict ? dict->value(wp) : 0;
            if (sn) {
#ifndef Q_OS_WINCE
                d->doWsaAsyncSelect(sn->fd, 0);
                d->active_fd[sn->fd].selected = false;
                d->postActivateSocketNotifiers();
#endif
                if (type < 3) {
                    QEvent event(QEvent::SockAct);
                    QCoreApplication::sendEvent(sn->obj, &event);
                } else {
                    QEvent event(QEvent::SockClose);
                    QCoreApplication::sendEvent(sn->obj, &event);
                }
            }
        }
        return 0;
#ifndef Q_OS_WINCE
    } else if (message == WM_QT_ACTIVATENOTIFIERS) {
        Q_ASSERT(d != 0);

        // register all socket notifiers
        for (QSFDict::iterator it = d->active_fd.begin(), end = d->active_fd.end();
             it != end; ++it) {
            QSockFd &sd = it.value();
            if (!sd.selected) {
                d->doWsaAsyncSelect(it.key(), sd.event);
                sd.selected = true;
            }
        }
        d->activateNotifiersPosted = false;
        return 0;
#endif // !Q_OS_WINCE
    } else if (message == WM_QT_SENDPOSTEDEVENTS
               // we also use a Windows timer to send posted events when the message queue is full
               || (message == WM_TIMER
                   && d->sendPostedEventsWindowsTimerId != 0
                   && wp == (uint)d->sendPostedEventsWindowsTimerId)) {
        const int localSerialNumber = d->serialNumber.load();
        if (localSerialNumber != d->lastSerialNumber) {
            d->lastSerialNumber = localSerialNumber;
            q->sendPostedEvents();
        }
        return 0;
    } else if (message == WM_TIMER) {
        Q_ASSERT(d != 0);
        d->sendTimerEvent(wp);
        return 0;
    }

    return DefWindowProc(hwnd, message, wp, lp);
}

static inline UINT inputTimerMask()
{
    UINT result = QS_TIMER | QS_INPUT | QS_RAWINPUT;
    // QTBUG 28513, QTBUG-29097, QTBUG-29435: QS_TOUCH, QS_POINTER became part of
    // QS_INPUT in Windows Kit 8. They should not be used when running on pre-Windows 8.
#if WINVER >= 0x0602
    if (QSysInfo::WindowsVersion < QSysInfo::WV_WINDOWS8)
        result &= ~(QS_TOUCH | QS_POINTER);
#endif //  WINVER > 0x0601
    return result;
}

LRESULT QT_WIN_CALLBACK qt_GetMessageHook(int code, WPARAM wp, LPARAM lp)
{
    QEventDispatcherWin32 *q = qobject_cast<QEventDispatcherWin32 *>(QAbstractEventDispatcher::instance());
    Q_ASSERT(q != 0);

    if (wp == PM_REMOVE) {
        if (q) {
            MSG *msg = (MSG *) lp;
            QEventDispatcherWin32Private *d = q->d_func();
            const int localSerialNumber = d->serialNumber.load();
            static const UINT mask = inputTimerMask();
            if (HIWORD(GetQueueStatus(mask)) == 0) {
                // no more input or timer events in the message queue, we can allow posted events to be sent normally now
                if (d->sendPostedEventsWindowsTimerId != 0) {
                    // stop the timer to send posted events, since we now allow the WM_QT_SENDPOSTEDEVENTS message
                    KillTimer(d->internalHwnd, d->sendPostedEventsWindowsTimerId);
                    d->sendPostedEventsWindowsTimerId = 0;
                }
                (void) d->wakeUps.fetchAndStoreRelease(0);
                if (localSerialNumber != d->lastSerialNumber
                    // if this message IS the one that triggers sendPostedEvents(), no need to post it again
                    && (msg->hwnd != d->internalHwnd
                        || msg->message != WM_QT_SENDPOSTEDEVENTS)) {
                    PostMessage(d->internalHwnd, WM_QT_SENDPOSTEDEVENTS, 0, 0);
                }
            } else if (d->sendPostedEventsWindowsTimerId == 0
                       && localSerialNumber != d->lastSerialNumber) {
                // start a special timer to continue delivering posted events while
                // there are still input and timer messages in the message queue
                d->sendPostedEventsWindowsTimerId = SetTimer(d->internalHwnd,
                                                             SendPostedEventsWindowsTimerId,
                                                             0, // we specify zero, but Windows uses USER_TIMER_MINIMUM
                                                             NULL);
                // we don't check the return value of SetTimer()... if creating the timer failed, there's little
                // we can do. we just have to accept that posted events will be starved
            }
        }
    }
#ifdef Q_OS_WINCE
    return 0;
#else
    return q->d_func()->getMessageHook ? CallNextHookEx(0, code, wp, lp) : 0;
#endif
}

// Provide class name and atom for the message window used by
// QEventDispatcherWin32Private via Q_GLOBAL_STATIC shared between threads.
struct QWindowsMessageWindowClassContext
{
    QWindowsMessageWindowClassContext();
    ~QWindowsMessageWindowClassContext();

    ATOM atom;
    wchar_t *className;
};

QWindowsMessageWindowClassContext::QWindowsMessageWindowClassContext()
    : atom(0), className(0)
{
    // make sure that multiple Qt's can coexist in the same process
    const QString qClassName = QStringLiteral("QEventDispatcherWin32_Internal_Widget")
        + QString::number(quintptr(qt_internal_proc));
    className = new wchar_t[qClassName.size() + 1];
    qClassName.toWCharArray(className);
    className[qClassName.size()] = 0;

    WNDCLASS wc;
    wc.style = 0;
    wc.lpfnWndProc = qt_internal_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = qWinAppInst();
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    atom = RegisterClass(&wc);
    if (!atom) {
        qErrnoWarning("%s RegisterClass() failed", qPrintable(qClassName));
        delete [] className;
        className = 0;
    }
}

QWindowsMessageWindowClassContext::~QWindowsMessageWindowClassContext()
{
    if (className) {
        UnregisterClass(className, qWinAppInst());
        delete [] className;
    }
}

Q_GLOBAL_STATIC(QWindowsMessageWindowClassContext, qWindowsMessageWindowClassContext)

static HWND qt_create_internal_window(const QEventDispatcherWin32 *eventDispatcher)
{
    QWindowsMessageWindowClassContext *ctx = qWindowsMessageWindowClassContext();
    if (!ctx->atom)
        return 0;
#ifdef Q_OS_WINCE
    HWND parent = 0;
#else
    HWND parent = HWND_MESSAGE;
#endif
    HWND wnd = CreateWindow(ctx->className,    // classname
                            ctx->className,    // window name
                            0,                 // style
                            0, 0, 0, 0,        // geometry
                            parent,            // parent
                            0,                 // menu handle
                            qWinAppInst(),     // application
                            0);                // windows creation data.

    if (!wnd) {
        qErrnoWarning("CreateWindow() for QEventDispatcherWin32 internal window failed");
        return 0;
    }

#ifdef GWLP_USERDATA
    SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)eventDispatcher);
#else
    SetWindowLong(wnd, GWL_USERDATA, (LONG)eventDispatcher);
#endif

    return wnd;
}

static void calculateNextTimeout(WinTimerInfo *t, quint64 currentTime)
{
    uint interval = t->interval;
    if ((interval >= 20000u && t->timerType != Qt::PreciseTimer) || t->timerType == Qt::VeryCoarseTimer) {
        // round the interval, VeryCoarseTimers only have full second accuracy
        interval = ((interval + 500)) / 1000 * 1000;
    }
    t->interval = interval;
    t->timeout = currentTime + interval;
}

void QEventDispatcherWin32Private::registerTimer(WinTimerInfo *t)
{
    Q_ASSERT(internalHwnd);

    Q_Q(QEventDispatcherWin32);

    int ok = 0;
    calculateNextTimeout(t, qt_msectime());
    uint interval = t->interval;
    if (interval == 0u) {
        // optimization for single-shot-zero-timer
        QCoreApplication::postEvent(q, new QZeroTimerEvent(t->timerId));
        ok = 1;
    } else if ((interval < 20u || t->timerType == Qt::PreciseTimer) && qtimeSetEvent) {
        ok = t->fastTimerId = qtimeSetEvent(interval, 1, qt_fast_timer_proc, (DWORD_PTR)t,
                                            TIME_CALLBACK_FUNCTION | TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
    }

    if (ok == 0) {
        // user normal timers for (Very)CoarseTimers, or if no more multimedia timers available
        ok = SetTimer(internalHwnd, t->timerId, interval, 0);
    }

    if (ok == 0)
        qErrnoWarning("QEventDispatcherWin32::registerTimer: Failed to create a timer");
}

void QEventDispatcherWin32Private::unregisterTimer(WinTimerInfo *t)
{
    if (t->interval == 0) {
        QCoreApplicationPrivate::removePostedTimerEvent(t->dispatcher, t->timerId);
    } else if (t->fastTimerId != 0) {
        qtimeKillEvent(t->fastTimerId);
        QCoreApplicationPrivate::removePostedTimerEvent(t->dispatcher, t->timerId);
    } else if (internalHwnd) {
        KillTimer(internalHwnd, t->timerId);
    }
    delete t;
}

void QEventDispatcherWin32Private::sendTimerEvent(int timerId)
{
    WinTimerInfo *t = timerDict.value(timerId);
    if (t && !t->inTimerEvent) {
        // send event, but don't allow it to recurse
        t->inTimerEvent = true;

        // recalculate next emission
        calculateNextTimeout(t, qt_msectime());

        QTimerEvent e(t->timerId);
        QCoreApplication::sendEvent(t->obj, &e);

        // timer could have been removed
        t = timerDict.value(timerId);
        if (t) {
            t->inTimerEvent = false;
        }
    }
}

void QEventDispatcherWin32Private::doWsaAsyncSelect(int socket, long event)
{
    Q_ASSERT(internalHwnd);
    // BoundsChecker may emit a warning for WSAAsyncSelect when event == 0
    // This is a BoundsChecker bug and not a Qt bug
    WSAAsyncSelect(socket, internalHwnd, event ? int(WM_QT_SOCKETNOTIFIER) : 0, event);
}

#ifndef Q_OS_WINCE
void QEventDispatcherWin32Private::postActivateSocketNotifiers()
{
    if (!activateNotifiersPosted)
        activateNotifiersPosted = PostMessage(internalHwnd, WM_QT_ACTIVATENOTIFIERS, 0, 0);
}
#endif // !Q_OS_WINCE

void QEventDispatcherWin32::createInternalHwnd()
{
    Q_D(QEventDispatcherWin32);

    if (d->internalHwnd)
        return;
    d->internalHwnd = qt_create_internal_window(this);

    installMessageHook();

    // start all normal timers
    for (int i = 0; i < d->timerVec.count(); ++i)
        d->registerTimer(d->timerVec.at(i));
}

void QEventDispatcherWin32::installMessageHook()
{
    Q_D(QEventDispatcherWin32);

    if (d->getMessageHook)
        return;

#ifndef Q_OS_WINCE
    // setup GetMessage hook needed to drive our posted events
    d->getMessageHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC) qt_GetMessageHook, NULL, GetCurrentThreadId());
    if (!d->getMessageHook) {
        int errorCode = GetLastError();
        qFatal("Qt: INTERNAL ERROR: failed to install GetMessage hook: %d, %s",
               errorCode, qPrintable(qt_error_string(errorCode)));
    }
#endif
}

void QEventDispatcherWin32::uninstallMessageHook()
{
    Q_D(QEventDispatcherWin32);

#ifndef Q_OS_WINCE
    if (d->getMessageHook)
        UnhookWindowsHookEx(d->getMessageHook);
#endif
    d->getMessageHook = 0;
}

QEventDispatcherWin32::QEventDispatcherWin32(QObject *parent)
    : QAbstractEventDispatcher(*new QEventDispatcherWin32Private, parent)
{
}

QEventDispatcherWin32::QEventDispatcherWin32(QEventDispatcherWin32Private &dd, QObject *parent)
    : QAbstractEventDispatcher(dd, parent)
{ }

QEventDispatcherWin32::~QEventDispatcherWin32()
{
}

bool QEventDispatcherWin32::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherWin32);

    if (!d->internalHwnd) {
        createInternalHwnd();
        wakeUp(); // trigger a call to sendPostedEvents()
    }

    d->interrupt = false;
    emit awake();

    bool canWait;
    bool retVal = false;
    bool seenWM_QT_SENDPOSTEDEVENTS = false;
    bool needWM_QT_SENDPOSTEDEVENTS = false;
    do {
        DWORD waitRet = 0;
        HANDLE pHandles[MAXIMUM_WAIT_OBJECTS - 1];
        QVarLengthArray<MSG> processedTimers;
        while (!d->interrupt) {
            DWORD nCount = d->winEventNotifierList.count();
            Q_ASSERT(nCount < MAXIMUM_WAIT_OBJECTS - 1);

            MSG msg;
            bool haveMessage;

            if (!(flags & QEventLoop::ExcludeUserInputEvents) && !d->queuedUserInputEvents.isEmpty()) {
                // process queued user input events
                haveMessage = true;
                msg = d->queuedUserInputEvents.takeFirst();
            } else if(!(flags & QEventLoop::ExcludeSocketNotifiers) && !d->queuedSocketEvents.isEmpty()) {
                // process queued socket events
                haveMessage = true;
                msg = d->queuedSocketEvents.takeFirst();
            } else {
                haveMessage = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
                if (haveMessage) {
                    if ((flags & QEventLoop::ExcludeUserInputEvents)
                        && ((msg.message >= WM_KEYFIRST
                             && msg.message <= WM_KEYLAST)
                            || (msg.message >= WM_MOUSEFIRST
                                && msg.message <= WM_MOUSELAST)
                            || msg.message == WM_MOUSEWHEEL
                            || msg.message == WM_MOUSEHWHEEL
                            || msg.message == WM_TOUCH
#ifndef QT_NO_GESTURES
                            || msg.message == WM_GESTURE
                            || msg.message == WM_GESTURENOTIFY
#endif
                            || msg.message == WM_CLOSE)) {
                        // queue user input events for later processing
                        d->queuedUserInputEvents.append(msg);
                        continue;
                    }
                    if ((flags & QEventLoop::ExcludeSocketNotifiers)
                        && (msg.message == WM_QT_SOCKETNOTIFIER && msg.hwnd == d->internalHwnd)) {
                        // queue socket events for later processing
                        d->queuedSocketEvents.append(msg);
                        continue;
                    }
                }
            }
            if (!haveMessage) {
                // no message - check for signalled objects
                for (int i=0; i<(int)nCount; i++)
                    pHandles[i] = d->winEventNotifierList.at(i)->handle();
                waitRet = MsgWaitForMultipleObjectsEx(nCount, pHandles, 0, QS_ALLINPUT, MWMO_ALERTABLE);
                if ((haveMessage = (waitRet == WAIT_OBJECT_0 + nCount))) {
                    // a new message has arrived, process it
                    continue;
                }
            }
            if (haveMessage) {
                // WinCE doesn't support hooks at all, so we have to call this by hand :(
                if (!d->getMessageHook)
                    (void) qt_GetMessageHook(0, PM_REMOVE, (LPARAM) &msg);

                if (d->internalHwnd == msg.hwnd && msg.message == WM_QT_SENDPOSTEDEVENTS) {
                    if (seenWM_QT_SENDPOSTEDEVENTS) {
                        // when calling processEvents() "manually", we only want to send posted
                        // events once
                        needWM_QT_SENDPOSTEDEVENTS = true;
                        continue;
                    }
                    seenWM_QT_SENDPOSTEDEVENTS = true;
                } else if (msg.message == WM_TIMER) {
                    // avoid live-lock by keeping track of the timers we've already sent
                    bool found = false;
                    for (int i = 0; !found && i < processedTimers.count(); ++i) {
                        const MSG processed = processedTimers.constData()[i];
                        found = (processed.wParam == msg.wParam && processed.hwnd == msg.hwnd && processed.lParam == msg.lParam);
                    }
                    if (found)
                        continue;
                    processedTimers.append(msg);
                } else if (msg.message == WM_QUIT) {
                    if (QCoreApplication::instance())
                        QCoreApplication::instance()->quit();
                    return false;
                }

                if (!filterNativeEvent(QByteArrayLiteral("windows_generic_MSG"), &msg, 0)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } else if (waitRet - WAIT_OBJECT_0 < nCount) {
                d->activateEventNotifier(d->winEventNotifierList.at(waitRet - WAIT_OBJECT_0));
            } else {
                // nothing todo so break
                break;
            }
            retVal = true;
        }

        // still nothing - wait for message or signalled objects
        canWait = (!retVal
                   && !d->interrupt
                   && (flags & QEventLoop::WaitForMoreEvents));
        if (canWait) {
            DWORD nCount = d->winEventNotifierList.count();
            Q_ASSERT(nCount < MAXIMUM_WAIT_OBJECTS - 1);
            for (int i=0; i<(int)nCount; i++)
                pHandles[i] = d->winEventNotifierList.at(i)->handle();

            emit aboutToBlock();
            waitRet = MsgWaitForMultipleObjectsEx(nCount, pHandles, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);
            emit awake();
            if (waitRet - WAIT_OBJECT_0 < nCount) {
                d->activateEventNotifier(d->winEventNotifierList.at(waitRet - WAIT_OBJECT_0));
                retVal = true;
            }
        }
    } while (canWait);

    if (!seenWM_QT_SENDPOSTEDEVENTS && (flags & QEventLoop::EventLoopExec) == 0) {
        // when called "manually", always send posted events
        sendPostedEvents();
    }

    if (needWM_QT_SENDPOSTEDEVENTS)
        PostMessage(d->internalHwnd, WM_QT_SENDPOSTEDEVENTS, 0, 0);

    return retVal;
}

bool QEventDispatcherWin32::hasPendingEvents()
{
    MSG msg;
    return qGlobalPostedEventsCount() || PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
}

void QEventDispatcherWin32::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
    int sockfd = notifier->socket();
    int type = notifier->type();
#ifndef QT_NO_DEBUG
    if (sockfd < 0) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be enabled from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherWin32);
    QSNDict *sn_vec[3] = { &d->sn_read, &d->sn_write, &d->sn_except };
    QSNDict *dict = sn_vec[type];

    if (QCoreApplication::closingDown()) // ### d->exitloop?
        return; // after sn_cleanup, don't reinitialize.

    if (dict->contains(sockfd)) {
        const char *t[] = { "Read", "Write", "Exception" };
    /* Variable "socket" below is a function pointer. */
        qWarning("QSocketNotifier: Multiple socket notifiers for "
                 "same socket %d and type %s", sockfd, t[type]);
    }

    createInternalHwnd();

    QSockNot *sn = new QSockNot;
    sn->obj = notifier;
    sn->fd  = sockfd;
    dict->insert(sn->fd, sn);

    long event = 0;
    if (d->sn_read.contains(sockfd))
        event |= FD_READ | FD_CLOSE | FD_ACCEPT;
    if (d->sn_write.contains(sockfd))
        event |= FD_WRITE | FD_CONNECT;
    if (d->sn_except.contains(sockfd))
        event |= FD_OOB;

    QSFDict::iterator it = d->active_fd.find(sockfd);
    if (it != d->active_fd.end()) {
        QSockFd &sd = it.value();
#ifndef Q_OS_WINCE
        if (sd.selected) {
            d->doWsaAsyncSelect(sockfd, 0);
            sd.selected = false;
        }
#endif // !Q_OS_WINCE
        sd.event |= event;
    } else {
        d->active_fd.insert(sockfd, QSockFd(event));
    }

#ifndef Q_OS_WINCE
    d->postActivateSocketNotifiers();
#else
    d->doWsaAsyncSelect(sockfd, event);
#endif
}

void QEventDispatcherWin32::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_ASSERT(notifier);
#ifndef QT_NO_DEBUG
    int sockfd = notifier->socket();
    if (sockfd < 0) {
        qWarning("QSocketNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QSocketNotifier: socket notifiers cannot be disabled from another thread");
        return;
    }
#endif
    doUnregisterSocketNotifier(notifier);
}

void QEventDispatcherWin32::doUnregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_D(QEventDispatcherWin32);
    int type = notifier->type();
    int sockfd = notifier->socket();
    Q_ASSERT(sockfd >= 0);

    QSFDict::iterator it = d->active_fd.find(sockfd);
    if (it != d->active_fd.end()) {
        QSockFd &sd = it.value();
#ifndef Q_OS_WINCE
        if (sd.selected)
            d->doWsaAsyncSelect(sockfd, 0);
        const long event[3] = { FD_READ | FD_CLOSE | FD_ACCEPT, FD_WRITE | FD_CONNECT, FD_OOB };
        sd.event ^= event[type];
        if (sd.event == 0) {
            d->active_fd.erase(it);
        } else if (sd.selected) {
            sd.selected = false;
            d->postActivateSocketNotifiers();
        }
#else
        const long event[3] = { FD_READ | FD_CLOSE | FD_ACCEPT, FD_WRITE | FD_CONNECT, FD_OOB };
        sd.event ^= event[type];
        d->doWsaAsyncSelect(sockfd, sd.event);
        if (sd.event == 0)
            d->active_fd.erase(it);
#endif // !Q_OS_WINCE
    }

    QSNDict *sn_vec[3] = { &d->sn_read, &d->sn_write, &d->sn_except };
    QSNDict *dict = sn_vec[type];
    QSockNot *sn = dict->value(sockfd);
    if (!sn)
        return;

    dict->remove(sockfd);
    delete sn;
}

void QEventDispatcherWin32::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QEventDispatcherWin32::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWin32::registerTimer: timers cannot be started from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherWin32);

    // exiting ... do not register new timers
    // (QCoreApplication::closingDown() is set too late to be used here)
    if (d->closingDown)
        return;

    WinTimerInfo *t = new WinTimerInfo;
    t->dispatcher = this;
    t->timerId  = timerId;
    t->interval = interval;
    t->timerType = timerType;
    t->obj  = object;
    t->inTimerEvent = false;
    t->fastTimerId = 0;

    if (d->internalHwnd)
        d->registerTimer(t);

    d->timerVec.append(t);                      // store in timer vector
    d->timerDict.insert(t->timerId, t);          // store timers in dict
}

bool QEventDispatcherWin32::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWin32::unregisterTimer: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (thread() != currentThread) {
        qWarning("QEventDispatcherWin32::unregisterTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherWin32);
    if (d->timerVec.isEmpty() || timerId <= 0)
        return false;

    WinTimerInfo *t = d->timerDict.value(timerId);
    if (!t)
        return false;

    d->timerDict.remove(t->timerId);
    d->timerVec.removeAll(t);
    d->unregisterTimer(t);
    return true;
}

bool QEventDispatcherWin32::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWin32::unregisterTimers: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (object->thread() != thread() || thread() != currentThread) {
        qWarning("QEventDispatcherWin32::unregisterTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherWin32);
    if (d->timerVec.isEmpty())
        return false;
    WinTimerInfo *t;
    for (int i=0; i<d->timerVec.size(); i++) {
        t = d->timerVec.at(i);
        if (t && t->obj == object) {                // object found
            d->timerDict.remove(t->timerId);
            d->timerVec.removeAt(i);
            d->unregisterTimer(t);
            --i;
        }
    }
    return true;
}

QList<QEventDispatcherWin32::TimerInfo>
QEventDispatcherWin32::registeredTimers(QObject *object) const
{
    if (!object) {
        qWarning("QEventDispatcherWin32:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }

    Q_D(const QEventDispatcherWin32);
    QList<TimerInfo> list;
    for (int i = 0; i < d->timerVec.size(); ++i) {
        const WinTimerInfo *t = d->timerVec.at(i);
        if (t && t->obj == object)
            list << TimerInfo(t->timerId, t->interval, t->timerType);
    }
    return list;
}

bool QEventDispatcherWin32::registerEventNotifier(QWinEventNotifier *notifier)
{
    if (!notifier) {
        qWarning("QWinEventNotifier: Internal error");
        return false;
    } else if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QWinEventNotifier: event notifiers cannot be enabled from another thread");
        return false;
    }

    Q_D(QEventDispatcherWin32);

    if (d->winEventNotifierList.contains(notifier))
        return true;

    if (d->winEventNotifierList.count() >= MAXIMUM_WAIT_OBJECTS - 2) {
        qWarning("QWinEventNotifier: Cannot have more than %d enabled at one time", MAXIMUM_WAIT_OBJECTS - 2);
        return false;
    }
    d->winEventNotifierList.append(notifier);
    return true;
}

void QEventDispatcherWin32::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    if (!notifier) {
        qWarning("QWinEventNotifier: Internal error");
        return;
    } else if (notifier->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QWinEventNotifier: event notifiers cannot be disabled from another thread");
        return;
    }

    Q_D(QEventDispatcherWin32);

    int i = d->winEventNotifierList.indexOf(notifier);
    if (i != -1)
        d->winEventNotifierList.takeAt(i);
}

void QEventDispatcherWin32::activateEventNotifiers()
{
    Q_D(QEventDispatcherWin32);
    //### this could break if events are removed/added in the activation
    for (int i=0; i<d->winEventNotifierList.count(); i++) {
#if !defined(Q_OS_WINCE)
        if (WaitForSingleObjectEx(d->winEventNotifierList.at(i)->handle(), 0, TRUE) == WAIT_OBJECT_0)
#else
        if (WaitForSingleObject(d->winEventNotifierList.at(i)->handle(), 0) == WAIT_OBJECT_0)
#endif
            d->activateEventNotifier(d->winEventNotifierList.at(i));
    }
}

int QEventDispatcherWin32::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWin32::remainingTime: invalid argument");
        return -1;
    }
#endif

    Q_D(QEventDispatcherWin32);

    if (d->timerVec.isEmpty())
        return -1;

    quint64 currentTime = qt_msectime();

    WinTimerInfo *t;
    for (int i=0; i<d->timerVec.size(); i++) {
        t = d->timerVec.at(i);
        if (t && t->timerId == timerId) {                // timer found
            if (currentTime < t->timeout) {
                // time to wait
                return t->timeout - currentTime;
            } else {
                return 0;
            }
        }
    }

#ifndef QT_NO_DEBUG
    qWarning("QEventDispatcherWin32::remainingTime: timer id %d not found", timerId);
#endif

    return -1;
}

void QEventDispatcherWin32::wakeUp()
{
    Q_D(QEventDispatcherWin32);
    d->serialNumber.ref();
    if (d->internalHwnd && d->wakeUps.testAndSetAcquire(0, 1)) {
        // post a WM_QT_SENDPOSTEDEVENTS to this thread if there isn't one already pending
        PostMessage(d->internalHwnd, WM_QT_SENDPOSTEDEVENTS, 0, 0);
    }
}

void QEventDispatcherWin32::interrupt()
{
    Q_D(QEventDispatcherWin32);
    d->interrupt = true;
    wakeUp();
}

void QEventDispatcherWin32::flush()
{ }

void QEventDispatcherWin32::startingUp()
{ }

void QEventDispatcherWin32::closingDown()
{
    Q_D(QEventDispatcherWin32);

    // clean up any socketnotifiers
    while (!d->sn_read.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_read.begin()))->obj);
    while (!d->sn_write.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_write.begin()))->obj);
    while (!d->sn_except.isEmpty())
        doUnregisterSocketNotifier((*(d->sn_except.begin()))->obj);
    Q_ASSERT(d->active_fd.isEmpty());

    // clean up any timers
    for (int i = 0; i < d->timerVec.count(); ++i)
        d->unregisterTimer(d->timerVec.at(i));
    d->timerVec.clear();
    d->timerDict.clear();

    d->closingDown = true;

    uninstallMessageHook();
}

bool QEventDispatcherWin32::event(QEvent *e)
{
    Q_D(QEventDispatcherWin32);
    if (e->type() == QEvent::ZeroTimerEvent) {
        QZeroTimerEvent *zte = static_cast<QZeroTimerEvent*>(e);
        WinTimerInfo *t = d->timerDict.value(zte->timerId());
        if (t) {
            t->inTimerEvent = true;

            QTimerEvent te(zte->timerId());
            QCoreApplication::sendEvent(t->obj, &te);

            t = d->timerDict.value(zte->timerId());
            if (t) {
                if (t->interval == 0 && t->inTimerEvent) {
                    // post the next zero timer event as long as the timer was not restarted
                    QCoreApplication::postEvent(this, new QZeroTimerEvent(zte->timerId()));
                }

                t->inTimerEvent = false;
            }
        }
        return true;
    } else if (e->type() == QEvent::Timer) {
        QTimerEvent *te = static_cast<QTimerEvent*>(e);
        d->sendTimerEvent(te->timerId());
    }
    return QAbstractEventDispatcher::event(e);
}

void QEventDispatcherWin32::sendPostedEvents()
{
    Q_D(QEventDispatcherWin32);
    QCoreApplicationPrivate::sendPostedEvents(0, 0, d->threadData);
}

QT_END_NAMESPACE

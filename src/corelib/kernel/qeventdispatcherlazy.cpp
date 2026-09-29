/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtCore module of the XD Toolkit.
**
** $QT_BEGIN_LICENSE:APACHE2$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeventdispatcherlazy.h"

#include <private/qcoreapplication_p.h>
#include <private/qthread_p.h>
#include <QtCore/qexception.h>


QEventDispatcherLazy::QEventDispatcherLazy(QObject *parent) Q_DECL_NOEXCEPT_EXPR(false)
    : super(parent)
    , lastError(Q_NULLPTR)
{
    decorAttach(this);
}

QEventDispatcherLazy::~QEventDispatcherLazy()
{
    // Nothing to do (but required).
}

void QEventDispatcherLazy::preDecorLoad()
{
    // Prevents recursion.
    PreDecorContext *ctx = new PreDecorContext();
    ctx->isUsedByApp = bool(QCoreApplicationPrivate::eventDispatcher == this);
    if (ctx->isUsedByApp) {
        QCoreApplicationPrivate::eventDispatcher = Q_NULLPTR;
    }
    ctx->thread = QThreadData::current();
    if (ctx->thread) {
        ctx->isUsedByThread = ctx->thread
                && ctx->thread->eventDispatcher.testAndSetOrdered(this, Q_NULLPTR);
    } else {
        // Thread should be already set.
        qThrowAtomicMismatch();
    }

    // Restores wherever used.
    decorListeners.prepend([this, ctx] (QObject *obj) {
        Q_UNUSED(obj)
        this->decorListener(ctx);
    });
}

void QEventDispatcherLazy::decorLoad()
{
    QObjectDecorLocker _(this);
    if ( ! this->decorLoaded) {
        preDecorLoad();

        QAbstractEventDispatcher *actual = Q_NULLPTR;
        QCoreApplication *app = qApp;
        if (app) {
            QCoreApplicationPrivate *d = QCoreApplicationPrivate::get(app);
            if ( ! QCoreApplicationPrivate::eventDispatcher) {
                (*d).QCoreApplicationPrivate::createEventDispatcher();
            }
            actual = QCoreApplicationPrivate::eventDispatcher;
        } else {
            // No application yet, as for a dispatcher installed before it
            // through QCoreApplication::setEventDispatcher(): creates the
            // default the way QThread creates its own, in the thread slot,
            // then takes it back out of there.
            QThreadData *thread = QThreadData::current();
            QAbstractEventDispatcher *held = thread->eventDispatcher.fetchAndStoreOrdered(Q_NULLPTR);
            QThreadPrivate::createEventDispatcher(thread);
            actual = thread->eventDispatcher.fetchAndStoreOrdered(held);
        }

        postDecorLoad(actual);
    }
}

void QEventDispatcherLazy::decorListener(PreDecorContext *ctx) {
    QAbstractEventDispatcher *dispatcher = reinterpret_cast<QAbstractEventDispatcher *>(decorLoaded.data());
    if (ctx->isUsedByThread) {
        Q_IF(ctx->thread) {
            ctx->thread->eventDispatcher.testAndSetOrdered(Q_NULLPTR, dispatcher);
        }
    }
    if (ctx->isUsedByApp) {
        //Q_ASSERT_X( ! QCoreApplicationPrivate::eventDispatcher,
        //           "Decor", "Should not set dispatcher until lazy-load completes.");
        QCoreApplicationPrivate::eventDispatcher = this;
    }

    delete ctx;
}

bool QEventDispatcherLazy::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    return toDecoratee()->processEvents(flags);
}

bool QEventDispatcherLazy::hasPendingEvents()
{
    return toDecoratee()->hasPendingEvents();
}

void QEventDispatcherLazy::registerSocketNotifier(QSocketNotifier *notifier)
{
    return toDecoratee()->registerSocketNotifier(notifier);
}

void QEventDispatcherLazy::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    return toDecoratee()->unregisterSocketNotifier(notifier);
}

void QEventDispatcherLazy::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
    return toDecoratee()->registerTimer(timerId, interval, timerType, object);
}

bool QEventDispatcherLazy::unregisterTimer(int timerId)
{
    return toDecoratee()->unregisterTimer(timerId);
}

bool QEventDispatcherLazy::unregisterTimers(QObject *object)
{
    return toDecoratee()->unregisterTimers(object);
}

QList<QAbstractEventDispatcher::TimerInfo> QEventDispatcherLazy::registeredTimers(QObject *object) const
{
    return toDecoratee()->registeredTimers(object);
}

int QEventDispatcherLazy::remainingTime(int timerId)
{
    return toDecoratee()->remainingTime(timerId);
}

#ifdef Q_OS_WIN
bool QEventDispatcherLazy::registerEventNotifier(QWinEventNotifier *notifier)
{
    return toDecoratee()->registerEventNotifier(notifier);
}

void QEventDispatcherLazy::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    return toDecoratee()->unregisterEventNotifier(notifier);
}
#endif // Q_OS_WIN

void QEventDispatcherLazy::wakeUp()
{
    // TRACE/QEventDispatcher/decor BugFix: skip until decoratee loads #1,
    // since `load` may still be constructing the platform integration --
    // e.g. `QCocoaInputContext`'s ctor (called from inside
    // `QCocoaIntegration`'s ctor, which is itself the body of `load`)
    // queues `QMetaObject::invokeMethod(... Qt::QueuedConnection)`, and
    // the resulting `QCoreApplication::postEvent` wakes the dispatcher.
    // Forcing `toDecoratee()` here would re-enter `load` and create a
    // second platform integration. Posted events are already in
    // `data->postEventList`, so the wake is a no-op until a real event
    // loop runs; safe to skip until the decoratee is in.
    if ( ! isDecorLoaded())
        return;
    return toDecoratee()->wakeUp();
}

void QEventDispatcherLazy::interrupt()
{
    // TRACE/QEventDispatcher/decor BugFix: skip until decoratee loads #2.
    if ( ! isDecorLoaded())
        return;
    return toDecoratee()->interrupt();
}

void QEventDispatcherLazy::flush()
{
    // TRACE/QEventDispatcher/decor BugFix: skip until decoratee loads #3.
    if ( ! isDecorLoaded())
        return;
    return toDecoratee()->flush();
}

void QEventDispatcherLazy::startingUp()
{
    return toDecoratee()->startingUp();
}

void QEventDispatcherLazy::closingDown()
{
    return toDecoratee()->closingDown();
}

void QEventDispatcherLazyFunc::decorLoad()
{
    QObjectDecorLocker _(this);
    if ( ! this->decorLoaded) {
        if (this->load) {
            preDecorLoad();
            QAbstractEventDispatcher *actual = this->load(this);
            postDecorLoad(actual);
        }
    }
}

bool QEventDispatcherLazyFunc::lazyEvent(QLazyEvent *event)
{
    switch (event->type()) {
    case QLazyEvent::Destroy:
        if (this->destroy) {
            this->destroy(this);
            return false;
        }
    }

    return super::lazyEvent(event);
}

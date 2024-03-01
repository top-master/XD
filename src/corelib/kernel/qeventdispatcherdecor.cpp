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

#include "qeventdispatcherdecor.h"

#include <private/qcoreapplication_p.h>
#include <private/qthread_p.h>
#include <QtCore/qexception.h>


QEventDispatcherDecor::QEventDispatcherDecor(QObject *parent) Q_DECL_NOEXCEPT_EXPR(false)
    : super(parent)
    , lastError(Q_NULLPTR)
{
    decorAttach(this);
}

QEventDispatcherDecor::~QEventDispatcherDecor()
{
    // Nothing to do (but required).
}

void QEventDispatcherDecor::preDecorLoad()
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

void QEventDispatcherDecor::decorLoad()
{
    QObjectDecorLocker _(this);
    if ( ! this->decorLoaded) {
        preDecorLoad();

        QCoreApplication *app = qApp;
        Q_ASSERT(app);
        QCoreApplicationPrivate *d = QCoreApplicationPrivate::get(app);
        if ( ! QCoreApplicationPrivate::eventDispatcher) {
            (*d).QCoreApplicationPrivate::createEventDispatcher();
        }

        QAbstractEventDispatcher *actual = QCoreApplicationPrivate::eventDispatcher;

        postDecorLoad(actual);
    }
}

void QEventDispatcherDecor::decorListener(PreDecorContext *ctx) {
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

bool QEventDispatcherDecor::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    return toDecoratee()->processEvents(flags);
}

bool QEventDispatcherDecor::hasPendingEvents()
{
    return toDecoratee()->hasPendingEvents();
}

void QEventDispatcherDecor::registerSocketNotifier(QSocketNotifier *notifier)
{
    return toDecoratee()->registerSocketNotifier(notifier);
}

void QEventDispatcherDecor::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    return toDecoratee()->unregisterSocketNotifier(notifier);
}

void QEventDispatcherDecor::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
    return toDecoratee()->registerTimer(timerId, interval, timerType, object);
}

bool QEventDispatcherDecor::unregisterTimer(int timerId)
{
    return toDecoratee()->unregisterTimer(timerId);
}

bool QEventDispatcherDecor::unregisterTimers(QObject *object)
{
    return toDecoratee()->unregisterTimers(object);
}

QList<QAbstractEventDispatcher::TimerInfo> QEventDispatcherDecor::registeredTimers(QObject *object) const
{
    return toDecoratee()->registeredTimers(object);
}

int QEventDispatcherDecor::remainingTime(int timerId)
{
    return toDecoratee()->remainingTime(timerId);
}

#ifdef Q_OS_WIN
bool QEventDispatcherDecor::registerEventNotifier(QWinEventNotifier *notifier)
{
    return toDecoratee()->registerEventNotifier(notifier);
}

void QEventDispatcherDecor::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    return toDecoratee()->unregisterEventNotifier(notifier);
}
#endif // Q_OS_WIN

void QEventDispatcherDecor::wakeUp()
{
    return toDecoratee()->wakeUp();
}

void QEventDispatcherDecor::interrupt()
{
    return toDecoratee()->interrupt();
}

void QEventDispatcherDecor::flush()
{
    return toDecoratee()->flush();
}

void QEventDispatcherDecor::startingUp()
{
    return toDecoratee()->startingUp();
}

void QEventDispatcherDecor::closingDown()
{
    return toDecoratee()->closingDown();
}

void QEventDispatcherDecorFunc::decorLoad()
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

bool QEventDispatcherDecorFunc::lazyEvent(QLazyEvent *event)
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

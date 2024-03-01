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

QObject *QEventDispatcherDecor::decorLoad()
{
    QMutexLocker _(&decorMutex);
    if ( ! this->decorLoaded) {
        QCoreApplication *app = qApp;
        Q_ASSERT(app);
        QCoreApplicationPrivate *d = QCoreApplicationPrivate::get(app);
        if ( ! QCoreApplicationPrivate::eventDispatcher) {
            (*d).QCoreApplicationPrivate::createEventDispatcher();
        }

        QAbstractEventDispatcher *actual = QCoreApplicationPrivate::eventDispatcher;

        if (QLazinessResolver::get(d_ptr_from(actual)) != &globalImmutable) {
            QLazinessResolver::throwDuplicate();
        }

        postDecorLoad(actual);
    }

    return this->decorLoaded;
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

QObject *QEventDispatcherDecorFunc::decorLoad()
{
    QMutexLocker _(&decorMutex);
    if ( ! this->decorLoaded) {
        QAbstractEventDispatcher *actual = this->load(this);
        postDecorLoad(actual);
    }
    return this->decorLoaded;
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

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

#ifndef QEVENTDISPATCHER_DECOR_H
#define QEVENTDISPATCHER_DECOR_H

#include <QtCore/qabstracteventdispatcher.h>
#include <QtCore/qobjectdecor.h>


QT_BEGIN_NAMESPACE

class QThreadData;

/*!
A run-time replaceable decorator around an event dispatcher.

Unlike @ref QEventDispatcherLazy (a lazy loader that dissolves into its
decoratee's private), this keeps its own identity on the plain @ref QObjectDecor
base and forwards every call to a delegate it holds. @ref replace swaps that
delegate at run time and returns the previous one, so a test can install this
once as the thread's dispatcher, swap in an overriding dispatcher for the
duration of a case, then swap the original back -- without touching Qt's own
setEventDispatcher workflow (whose default stays @ref QEventDispatcherLazy).
*/
class Q_CORE_EXPORT QEventDispatcherDecor : public QAbstractEventDispatcher, public QObjectDecor
{
    Q_OBJECT
    typedef QAbstractEventDispatcher super;
    typedef QEventDispatcherDecor Self;
public:
    explicit QEventDispatcherDecor(QAbstractEventDispatcher *delegate, QObject *parent = Q_NULLPTR);
    ~QEventDispatcherDecor();

    // MARK: helpers.

    inline QAbstractEventDispatcher *toDecoratee() const {
        return reinterpret_cast<QAbstractEventDispatcher * >(QObjectDecor::toDecoratee().data());
    }

    /// Swaps the held delegate for @p newValue and returns the previous one.
    QAbstractEventDispatcher *replace(QAbstractEventDispatcher *newValue);

    // MARK: interface copy.

    bool processEvents(QEventLoop::ProcessEventsFlags flags) Q_DECL_OVERRIDE;
    bool hasPendingEvents() Q_DECL_OVERRIDE;

    void registerSocketNotifier(QSocketNotifier *notifier) Q_DECL_OVERRIDE;
    void unregisterSocketNotifier(QSocketNotifier *notifier) Q_DECL_OVERRIDE;

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) Q_DECL_OVERRIDE;
    bool unregisterTimer(int timerId) Q_DECL_OVERRIDE;
    bool unregisterTimers(QObject *object) Q_DECL_OVERRIDE;
    QList<TimerInfo > registeredTimers(QObject *object) const Q_DECL_OVERRIDE;

    int remainingTime(int timerId) Q_DECL_OVERRIDE;

#ifdef Q_OS_WIN
    bool registerEventNotifier(QWinEventNotifier *notifier) Q_DECL_OVERRIDE;
    void unregisterEventNotifier(QWinEventNotifier *notifier) Q_DECL_OVERRIDE;
#endif

    void wakeUp() Q_DECL_OVERRIDE;
    void interrupt() Q_DECL_OVERRIDE;
    void flush() Q_DECL_OVERRIDE;

    void startingUp() Q_DECL_OVERRIDE;
    void closingDown() Q_DECL_OVERRIDE;

protected:
    void decorLoad() Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QEventDispatcherDecor);

    QAbstractEventDispatcher *initialDelegate;
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_DECOR_H

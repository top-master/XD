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
#include <QtCore/qfunction.h>


QT_BEGIN_NAMESPACE

class QThreadData;

/**
 * Decorates QAbstractEventDispatcher with lazy-loading.
 *
 * This should work fine as long as @ref qobject_cast is used instead of the
 * raw @c reinterpret_cast.
 */
class Q_CORE_EXPORT QEventDispatcherDecor : public QAbstractEventDispatcher, public QObjectDecor
{
    Q_OBJECT
    typedef QAbstractEventDispatcher super;
    typedef QEventDispatcherDecor Self;
public:
    explicit QEventDispatcherDecor(QObject *parent = Q_NULLPTR) Q_DECL_NOEXCEPT_EXPR(false);
    ~QEventDispatcherDecor();

    // MARK: helpers.

    inline QAbstractEventDispatcher *toDecoratee() const {
        return reinterpret_cast<QAbstractEventDispatcher *>(QObjectDecor::toDecoratee().data());
    }

    // MARK: interface copy.

    bool processEvents(QEventLoop::ProcessEventsFlags flags) Q_DECL_OVERRIDE;
    bool hasPendingEvents() Q_DECL_OVERRIDE;

    void registerSocketNotifier(QSocketNotifier *notifier) Q_DECL_OVERRIDE;
    void unregisterSocketNotifier(QSocketNotifier *notifier) Q_DECL_OVERRIDE;

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) Q_DECL_OVERRIDE;
    bool unregisterTimer(int timerId) Q_DECL_OVERRIDE;
    bool unregisterTimers(QObject *object) Q_DECL_OVERRIDE;
    QList<TimerInfo> registeredTimers(QObject *object) const Q_DECL_OVERRIDE;

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

    // MARK: internals.

protected:
    inline QEventDispatcherDecor(QAbstractEventDispatcherPrivate &d, QObject *parent)
        : super(d, parent)
    {}

    void preDecorLoad() Q_DECL_OVERRIDE;
    void decorLoad() Q_DECL_OVERRIDE;

    struct PreDecorContext {
        Q_DECL_CONSTEXPR inline PreDecorContext()
            : thread(Q_NULLPTR)
            , isUsedByApp(false)
            , isUsedByThread(false)
        {}

        QThreadData *thread;
        bool isUsedByApp;
        bool isUsedByThread;
    };
    void decorListener(PreDecorContext *);

private:
    Q_DISABLE_COPY(QEventDispatcherDecor);

public:
    QErrorFunc lastError;
};

class Q_CORE_EXPORT QEventDispatcherDecorFunc : public QEventDispatcherDecor {
    typedef QEventDispatcherDecor super;
    typedef QEventDispatcherDecorFunc Self;
public:
    inline QEventDispatcherDecorFunc(QObject *parent = Q_NULLPTR)
        : super(parent)
    {}

    void decorLoad() Q_DECL_OVERRIDE;

    QFunction<QAbstractEventDispatcher *(QEventDispatcherDecor *)> load;
    QFunction<bool(QEventDispatcherDecor *)> destroy;

protected:
    bool lazyEvent(QLazyEvent *event) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QEventDispatcherDecorFunc)
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_DECOR_H

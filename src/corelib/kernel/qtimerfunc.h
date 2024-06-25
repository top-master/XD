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

#ifndef QTIMER_FUNC_H
#define QTIMER_FUNC_H

#ifndef QT_NO_QOBJECT

#include <QtCore/qobject.h>
#include <QtCore/qfunction.h>
#include <QtCore/qabstracteventdispatcher.h>

class Q_CORE_EXPORT QTimerFunc : public QObject
{
    QFunction<void () > m_callback;
    QAbstractEventDispatcher::TimerInfo m_info;
    bool m_isSingleShot;
public:
    inline QTimerFunc(int milliSec, const std::function<void()> &callback, Qt::TimerType timerType = Qt::CoarseTimer)
        : QObject(QAbstractEventDispatcher::instance())
        , m_callback(callback)
        , m_info(this->startTimer(milliSec, timerType), milliSec, timerType)
        , m_isSingleShot(false)
    {
    }

    ~QTimerFunc();

    QAbstractEventDispatcher::TimerInfo toInfo() const { return m_info; }

    /// Always @c false, unless created by @ref singleShot.
    ///
    /// WARNING: there is no setter for this property, since
    /// the class auto-deletes itself if single-shot mode is enabled.
    inline bool isSingleShot() const { return m_isSingleShot; }

    /// Similar to passing Lambda to @ref QTimer::singleShot but this's faster,
    /// since QTimer is more optimized for signal-and-slot things, this for Lambda.
    ///
    /// @warning Calling thread should have a running event-loop,
    /// otherwise, the given @p callback simply never gets called.
    static inline void singleShot(int milliSec, const std::function<void()> &callback, Qt::TimerType timerType = Qt::CoarseTimer) {
        QTimerFunc *that = new QTimerFunc(milliSec, callback, timerType);
        Q_CHECK_PTR(that);
        that->m_isSingleShot = true;
    }

protected:
    void timerEvent(QTimerEvent *) Q_DECL_OVERRIDE;
};

#endif // QT_NO_QOBJECT

#endif // QTIMER_FUNC_H
